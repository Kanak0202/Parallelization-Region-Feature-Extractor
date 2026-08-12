// RegionOutliner.cpp
#include "RegionOutliner.h"
#include "LoopUtils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Expr.h>
#include <clang/Lex/Lexer.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallPtrSet.h>

namespace {

// In the anonymous namespace, alongside FreeVariableCollector:
class InductionVarCollector
    : public clang::RecursiveASTVisitor<InductionVarCollector>
{
public:
    clang::SourceManager &SM;
    clang::SourceLocation RegionStart, RegionEnd;
    llvm::SmallPtrSet<clang::VarDecl*, 8> inductionVars;

    InductionVarCollector(clang::SourceManager &SM,
                           clang::SourceLocation Start,
                           clang::SourceLocation End)
        : SM(SM), RegionStart(Start), RegionEnd(End) {}

    bool inRegion(clang::SourceLocation Loc) const {
        return !SM.isBeforeInTranslationUnit(Loc, RegionStart) &&
               !SM.isBeforeInTranslationUnit(RegionEnd, Loc);
    }

    bool VisitForStmt(clang::ForStmt *FS) {
        if (!inRegion(FS->getBeginLoc())) return true;
        if (auto *VD = getInductionVar(FS))
            inductionVars.insert(VD);
        return true;
    }
};

class FreeVariableCollector
    : public clang::RecursiveASTVisitor<FreeVariableCollector>
{
public:
    clang::SourceManager &SM;
    clang::SourceLocation RegionStart, RegionEnd;
    llvm::SmallSetVector<clang::VarDecl*, 16> freeVars;
    llvm::SmallPtrSet<clang::VarDecl*, 16> writtenVars;

    FreeVariableCollector(clang::SourceManager &SM,
                           clang::SourceLocation Start,
                           clang::SourceLocation End)
        : SM(SM), RegionStart(Start), RegionEnd(End) {}

    bool inRegion(clang::SourceLocation Loc) const {
        return !SM.isBeforeInTranslationUnit(Loc, RegionStart) &&
               !SM.isBeforeInTranslationUnit(RegionEnd, Loc);
    }

    bool VisitDeclRefExpr(clang::DeclRefExpr *DRE) {
        if (!inRegion(DRE->getLocation())) return true;
        if (auto *VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl()))
            if (SM.isBeforeInTranslationUnit(VD->getLocation(), RegionStart))
                freeVars.insert(VD);
        return true;
    }

    bool VisitBinaryOperator(clang::BinaryOperator *BO) {
        if (!inRegion(BO->getBeginLoc())) return true;
        if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
            return true;
        auto *LHS = BO->getLHS()->IgnoreParenImpCasts();
        if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(LHS))
            if (auto *VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl()))
                writtenVars.insert(VD);
        return true;
    }
};

std::string extractPreamble(clang::SourceManager &SM, clang::FileID FID,
                             unsigned enclosingFuncLine,
                             bool suppressDefines = false)
{
    bool invalid = false;
    llvm::StringRef Buf = SM.getBufferData(FID, &invalid);
    if (invalid) return "";

    std::istringstream stream(Buf.str());
    std::string line, preamble;
    unsigned lineNo = 0;
    while (std::getline(stream, line))
    {
        // std::cerr << "[LINE " << lineNo << "] " << line << "\n";
        ++lineNo;
        if (lineNo >= enclosingFuncLine) break;
        size_t p = line.find_first_not_of(" \t\r");
        if (p == std::string::npos) continue;
        std::string trimmed = line.substr(p);

        std::string afterHash;
        if (!trimmed.empty() && trimmed[0] == '#')
        {
            size_t d = trimmed.find_first_not_of(" \t", 1);
            if (d != std::string::npos)
                afterHash = trimmed.substr(d);
        }

        bool isInclude = afterHash.rfind("include", 0) == 0;
        bool isDefine  = afterHash.rfind("define", 0) == 0;

        if (isInclude || (isDefine && !suppressDefines))
        {
            // std::cerr << "[PREAMBLE] " << trimmed << "\n";
            preamble += line + "\n";
            continue;
        }
        if (isDefine)
            continue; // handled via RegionDetector::getMacroPreamble instead

        // std::cerr << "Checking prototype candidate: [" << line << "]\n";
        trimmed = line.substr(p);

        static const std::regex PrototypeRegex(
            R"(^(extern|static|inline|const|volatile|unsigned|signed|short|long|void|char|int|float|double|bool|struct|union|enum)[^;]*\([^;]*\)\s*;$)"
        );

        if (std::regex_match(trimmed, PrototypeRegex))
        {
            preamble += line + "\n";
        }
    }
    return preamble;
}

// True for a genuine array type (`double a[50][50]` as a local/global),
// AND for a decayed multi-dim array parameter (`double a[50][50]` as a
// function parameter, which C adjusts to `double (*a)[50]` -- i.e. a
// PointerType whose pointee is itself an ArrayType). Both need the same
// `BaseType (*name)[dims...]` reconstruction; only genuine single-level
// pointers (`double *a`) should fall through to the plain pointer case.
static bool isArrayOrDecayedArrayParam(clang::QualType QT)
{
    if (QT->isArrayType())
        return true;
    if (QT->isPointerType() && QT->getPointeeType()->isArrayType())
        return true;
    return false;
}

std::string paramTypeString(clang::VarDecl *VD, bool isWritten)
{
    clang::QualType QT = VD->getType();

    if (isArrayOrDecayedArrayParam(QT))
    {
        // Start from the array type itself: either QT directly (true
        // array case) or QT's pointee (decayed array-parameter case).
        clang::QualType ArrayQT = QT->isArrayType() ? QT : QT->getPointeeType();

        // Recurse through all dimensions to find the base scalar type,
        // then reconstruct as `BaseType (*name)[dim2][dim3]...` --
        // i.e. decay only the OUTERMOST dimension to a pointer, keep
        // inner dimensions as array bounds so a[i][j] indexing inside
        // the outlined body still computes the correct stride.
        std::vector<std::string> dims;
        clang::QualType Cur = ArrayQT;
        while (Cur->isConstantArrayType())
        {
            auto *CAT = VD->getASTContext().getAsConstantArrayType(Cur);
            dims.push_back(std::to_string(CAT->getSize().getZExtValue()));
            Cur = CAT->getElementType();
        }
        // std::cerr << "  [DEBUG paramTypeString] VD=" << VD->getNameAsString()
        //           << " QT=" << QT.getAsString()
        //           << " isArrayType=" << QT->isArrayType()
        //           << " isPointerType=" << QT->isPointerType()
        //           << " dims.size()=" << dims.size();
        // for (auto &d : dims) std::cerr << " dim=" << d;
        // std::cerr << " baseType=" << Cur.getAsString() << "\n";

        // Cur is now the base scalar type (e.g. double)
        std::string result = Cur.getAsString() + " (* restrict " + VD->getNameAsString() + ")";
        // If QT was itself a genuine array type, dims[0] is the OUTER
        // dimension that we're decaying to a pointer here -- skip it,
        // print only the inner dims. If QT was already a decayed
        // pointer-to-array parameter, the outer dimension was already
        // stripped by C before we ever saw QT, so every entry in dims
        // is a *remaining* dimension and must be printed -- none skipped.
        size_t startIdx = QT->isArrayType() ? 1 : 0;
        for (size_t d = startIdx; d < dims.size(); ++d)
            result += "[" + dims[d] + "]";
        return result; // NOTE: caller must NOT append " " + name again for this case
    }
    if (QT->isPointerType())
    {
        std::string result = QT.getAsString();

        // Insert restrict after the existing '*'
        size_t star = result.find('*');
        if (star != std::string::npos)
            result.insert(star + 1, " restrict");

        return result;
    }

    return isWritten ? QT.getAsString() + "* restrict" : QT.getAsString();
}

std::string rewriteScalarRefs(std::string body,
                               const std::vector<std::string> &scalarNames)
{
    for (const auto &name : scalarNames)
    {
        std::regex wordRe("\\b" + name + "\\b");
        body = std::regex_replace(body, wordRe, "(*" + name + ")");
    }
    return body;
}

class ReturnStmtCollector
    : public clang::RecursiveASTVisitor<ReturnStmtCollector>
{
public:
    clang::SourceManager &SM;
    clang::SourceLocation RegionStart, RegionEnd;

    std::vector<clang::ReturnStmt *> returns;

    ReturnStmtCollector(clang::SourceManager &SM,
                        clang::SourceLocation Start,
                        clang::SourceLocation End)
        : SM(SM), RegionStart(Start), RegionEnd(End) {}

    bool inRegion(clang::SourceLocation Loc) const
    {
        return !SM.isBeforeInTranslationUnit(Loc, RegionStart) &&
               !SM.isBeforeInTranslationUnit(RegionEnd, Loc);
    }

    bool VisitReturnStmt(clang::ReturnStmt *RS)
    {
        if (inRegion(RS->getBeginLoc()))
            returns.push_back(RS);

        return true;
    }
};

} // namespace

std::string RegionOutliner::outlineRegion(
    ProfitabilityRegion &region,
    clang::FunctionDecl *EnclosingFD,
    clang::ASTContext *Context,
    const std::string &outputDir,
    unsigned regionIndex,
    RegionDetector *detector)
{
    auto &SM = Context->getSourceManager();
    auto &LangOpts = Context->getLangOpts();

    unsigned beginLine = region.getBeginLine(SM);
    unsigned endLine = region.getEndLine(SM);
    // std::cerr << "  [outliner] regionIndex=" << regionIndex
    //       << " beginLine=" << beginLine << " endLine=" << endLine << "\n";
    clang::FileID FID = SM.getFileID(region.getBegin());

    clang::SourceLocation bodyStart = SM.translateLineCol(FID, beginLine + 1, 1);
    clang::SourceLocation bodyEnd   = SM.translateLineCol(FID, endLine, 1);
    if (bodyStart.isInvalid() || bodyEnd.isInvalid()) {
        std::cerr << "  [outliner] FAILED: could not translate line/col to SourceLocation\n";
        return "";
    }

    bool invalid = false;
    std::string bodyText = clang::Lexer::getSourceText(
        clang::CharSourceRange::getCharRange(bodyStart, bodyEnd),
        SM, LangOpts, &invalid).str();

    // std::cerr
    // << "\n===== BODY =====\n"
    // << bodyText
    // << "\n===============\n";

    if (invalid) {
        std::cerr << "  [outliner] FAILED: Lexer::getSourceText reported invalid range\n";
        return "";
    }

    // std::cerr << "  [outliner] bodyStart resolves to line "
    //       << SM.getSpellingLineNumber(bodyStart)
    //       << ", bodyEnd resolves to line "
    //       << SM.getSpellingLineNumber(bodyEnd) << "\n";
    // std::cerr << "  [outliner] RAW bodyText for regionIndex=" << regionIndex
    //       << ":\n>>>\n" << bodyText << "\n<<<\n";

    if (outputDir.empty()) {
        std::cerr << "  [outliner] FAILED: outputDir is empty -- was it wired through the constructor?\n";
        return "";
    }

    FreeVariableCollector collector(SM, bodyStart, bodyEnd);
    collector.TraverseStmt(EnclosingFD->getBody());

    InductionVarCollector indCollector(SM, bodyStart, bodyEnd);
    indCollector.TraverseStmt(EnclosingFD->getBody());

    ReturnStmtCollector returnCollector(SM, bodyStart, bodyEnd);
    returnCollector.TraverseStmt(EnclosingFD->getBody());

    std::vector<std::string> params;
    std::vector<std::string> localDecls;
    std::vector<std::string> scalarRefNames;

    for (clang::VarDecl *VD : collector.freeVars)
    {
        if (indCollector.inductionVars.count(VD))
        {
            // Loop-control variable: declare locally instead of
            // capturing as an external parameter. Its value after the
            // region isn't observed by anything the outlined function
            // does, so this is safe for feature-extraction/timing purposes.
            localDecls.push_back(VD->getType().getAsString() + " " +
                                  VD->getNameAsString() + ";");
            continue;
        }

        bool written = collector.writtenVars.count(VD) > 0;

        if (isArrayOrDecayedArrayParam(VD->getType())) {
            params.push_back(paramTypeString(VD, written));
        } else {
            params.push_back(paramTypeString(VD, written) + " " + VD->getNameAsString());
        }

        bool isScalarByRef = !VD->getType()->isArrayType() &&
                              !VD->getType()->isPointerType() &&
                              written;
        if (isScalarByRef)
            scalarRefNames.push_back(VD->getNameAsString());
    }

    bodyText = rewriteScalarRefs(bodyText, scalarRefNames);
    bool hasReturnStmt = !returnCollector.returns.empty();

    std::string funcName = "capc_region_" + std::to_string(regionIndex);
    
    std::string returnType =
        hasReturnStmt ? EnclosingFD->getReturnType().getAsString()
                      : "void";

    std::ostringstream out;
    unsigned enclosingFuncLine = SM.getSpellingLineNumber(EnclosingFD->getBeginLoc());
    std::string macroPreamble = detector
        ? detector->getMacroPreamble(FID, enclosingFuncLine)
        : "";

    out << extractPreamble(SM, FID, enclosingFuncLine, detector != nullptr)
        << macroPreamble << "\n";
    out << returnType << " " << funcName << "(";
    for (size_t i = 0; i < params.size(); ++i)
    {
        out << params[i];
        if (i + 1 < params.size()) out << ", ";
    }
    out << ")" << "\n" << "{" << "\n";
    for (const auto &decl : localDecls)
        out << "    " << decl << "\n";
    
        out << bodyText << "\n}\n";

    std::string outPath = outputDir + "/" + funcName + ".c";
//     std::cerr
// << "\n================ FINAL OUTPUT ================\n"
// << out.str()
// << "\n==============================================\n";
    std::ofstream ofs(outPath);
    if (!ofs) {
        std::cerr << "  [outliner] FAILED: could not open '" << outPath
                   << "' for writing (check directory exists / permissions)\n";
        return "";
    }
    ofs << out.str();
    ofs.close();

    std::ifstream ifs(outPath);

// std::cerr
// << "\n=========== FILE CONTENT ===========\n"
// << ifs.rdbuf()
// << "\n====================================\n";

    region.setOutlinedInfo(outPath, funcName);
    return outPath;
}