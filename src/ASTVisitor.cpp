#include "ASTVisitor.h"

#include <iostream>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include "ASTFeatureExtractor.h"

ASTVisitor::ASTVisitor(
    clang::ASTContext *Context,
    LoopManager& manager,
    RegionDetector* regionDetector,
    std::string outputDir)
    : Context(Context), manager(manager), regionDetector(regionDetector),
      outputDir(std::move(outputDir))
{
}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *FD)
{
    if (FD->hasBody())
    {
        std::cout << "Function: "
                  << FD->getNameAsString()
                  << std::endl;

        if (regionDetector)
        {
            auto &SM = Context->getSourceManager();
            for (auto &region : regionDetector->getRegions())
            {
                if (!SM.isBeforeInTranslationUnit(region.getBegin(), FD->getBeginLoc()) &&
                    !SM.isBeforeInTranslationUnit(FD->getEndLoc(), region.getEnd()))
                {
                    std::string path = RegionOutliner::outlineRegion(
                        region, FD, Context, outputDir, regionCounter++, regionDetector);
                    if (!path.empty())
                        std::cout << "Outlined region to: " << path << std::endl;
                }
            }
        }
    }

    return true;
}

bool ASTVisitor::TraverseForStmt(clang::ForStmt *FS)
{
    loopStack.push_back(FS);
    bool result = clang::RecursiveASTVisitor<ASTVisitor>::TraverseForStmt(FS);
    loopStack.pop_back();
    return result;
}

bool ASTVisitor::VisitForStmt(clang::ForStmt *FS)
{
    auto &SM = Context->getSourceManager();

    LoopInfo info;
    ASTFeatureExtractor::extractFeatures(info, FS, Context);
    manager.addLoop(info);

    if (regionDetector)
    {
        ProfitabilityRegion *region = regionDetector->findRegion(info.lineNumber);
        if (region)
        {
            unsigned begin = region->getBeginLine(SM);
            unsigned end = region->getEndLine(SM);

            // Top-level for THIS region means: no ancestor loop currently
            // on the stack is itself inside this region's line range.
            // Overall AST nesting depth is irrelevant -- an enclosing
            // loop outside the pragma (e.g. an outer time-step loop)
            // must not disqualify this loop from being the region's
            // top-level entry.
            bool hasAncestorInSameRegion = false;
            for (size_t i = 0; i + 1 < loopStack.size(); ++i) // exclude self (back())
            {
                unsigned ancestorLine =
                    SM.getSpellingLineNumber(loopStack[i]->getBeginLoc());
                if (ancestorLine >= begin && ancestorLine <= end)
                {
                    hasAncestorInSameRegion = true;
                    break;
                }
            }

            if (!hasAncestorInSameRegion)
            {
                LoopInfo regionInfo = info;
                regionInfo.features.loopDepth =
                    1 + ASTFeatureExtractor::computeMaxNestingDepth(FS->getBody());
                regionInfo.features.iterationSpace =
                    ASTFeatureExtractor::computeNestedIterationSpace(FS, Context);

                region->addLoop(regionInfo);
                std::cout << "Loop added to region (top-level, nest depth "
                          << regionInfo.features.loopDepth << ").\n";
            }
            else
            {
                std::cout << "Loop nested inside a loop already registered "
                             "to this region -- skipping duplicate entry.\n";
            }
        }
    }

    return true;
}