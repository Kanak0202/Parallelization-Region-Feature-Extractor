#include "ASTFeatureExtractor.h"
#include "LoopUtils.h"

#include<clang/AST/RecursiveASTVisitor.h>
#include<clang/AST/Expr.h>
#include<clang/AST/Stmt.h>

#include <clang/Basic/SourceManager.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <llvm/ADT/SmallPtrSet.h>

//
//Helper visitor for counting function calls
//
class FunctionCallVisitor: public clang::RecursiveASTVisitor<FunctionCallVisitor>{

public: 
   unsigned count = 0;
   bool VisitCallExpr(clang::CallExpr *){
	count++;
	return true;
}
};


//
//Helper visitor for counting array accesses
//
class ArrayVisitor: public clang::RecursiveASTVisitor<ArrayVisitor>{

public:
   llvm::SmallPtrSet<const clang::ValueDecl*, 8> arrays;
   unsigned indirectCount = 0;
   bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr *ASE){
        if (const clang::ValueDecl *Base = getBaseDecl(ASE))
	   arrays.insert(Base);

	if (containsSubscript(ASE->getIdx()))
	   indirectCount++;

        return true;
}

private:
   const clang::ValueDecl* getBaseDecl(clang::ArraySubscriptExpr *ASE){
	clang::Expr *Base = ASE->getBase()->IgnoreParenImpCasts();
	if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(Base))
	   return DRE->getDecl();
	if (auto *InnerASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(Base))
	   return getBaseDecl(InnerASE); //multi-dim: A[i][j];
	return nullptr;
}

bool containsSubscript(clang::Expr *E){
   struct Finder : clang::RecursiveASTVisitor<Finder>{
	bool Found = false;
	bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr*){
	   Found = true;
	   return true;
}
} F;
F.TraverseStmt(E);
return F.Found;

}
};


//----------------------
//Existing Functions Start Here
//----------------------



void ASTFeatureExtractor::extractFeatures(
    LoopInfo &info,
    clang::ForStmt *FS,
    clang::ASTContext *Context)
{
    extractLocation(info, FS, Context);
    extractLoopDepth(info, FS, Context);
    extractIterationSpace(info, FS, Context);
    extractFunctionCalls(info, FS);
    extractArraysAccessed(info, FS);
    extractReductionVariables(info, FS, Context);

}

void ASTFeatureExtractor::extractLocation(
    LoopInfo &info,
    clang::ForStmt *FS,
    clang::ASTContext *Context)
{
    auto &SM = Context->getSourceManager();

    info.lineNumber =
        SM.getSpellingLineNumber(FS->getForLoc());

    info.columnNumber =
        SM.getSpellingColumnNumber(FS->getForLoc());
}

void ASTFeatureExtractor::extractLoopDepth(
    LoopInfo &info,
    clang::ForStmt *FS,
    clang::ASTContext *Context)
{
    unsigned depth = 1;

    const clang::Stmt *current = FS;

    while (true)
    {
        auto parents = Context->getParents(*current);

        if (parents.empty())
            break;

        const auto &parent = parents[0];

        if (const auto *parentStmt =
                parent.get<clang::Stmt>())
        {
            if (llvm::isa<clang::ForStmt>(parentStmt))
            {
                depth++;
            }

            current = parentStmt;
        }
        else
        {
            break;
        }
    }

    info.features.loopDepth = depth;
}

void ASTFeatureExtractor::extractFunctionCalls(
    LoopInfo &info,
    clang::ForStmt *FS) {
    FunctionCallVisitor visitor;
    visitor.TraverseStmt(FS->getBody());
    info.features.functionCalls = visitor.count;
}


void ASTFeatureExtractor::extractArraysAccessed(
    LoopInfo &info,
    clang::ForStmt *FS) {
    ArrayVisitor visitor;
    visitor.TraverseStmt(FS->getBody());
    info.features.arraysAccessed = visitor.arrays.size();
    info.features.indirectAccesses = visitor.indirectCount;
}

//
//Helper visitor for reductions
//

class ReductionVisitor: public clang::RecursiveASTVisitor<ReductionVisitor>{
public:
    clang::ASTContext *Context = nullptr;
    llvm::SmallPtrSet<const clang::ValueDecl*, 8> reductionVars;
    const clang::VarDecl *innermostInductionVar = nullptr; // set by caller

    bool VisitCompoundAssignOperator(clang::CompoundAssignOperator *CAO) {
        classifyScalar(CAO->getLHS());
        classifyArray(CAO->getLHS(), CAO->getRHS());
        return true;
    }

    bool VisitBinaryOperator(clang::BinaryOperator *BO) {
        if (BO->getOpcode() != clang::BO_Assign) return true;

        clang::Expr *LHS = BO->getLHS()->IgnoreParenImpCasts();

        if (auto *LHSRef = llvm::dyn_cast<clang::DeclRefExpr>(LHS)) {

    // Existing arithmetic reduction detection
    if (containsSameDeclRef(BO->getRHS(), LHSRef->getDecl())) {
        classifyScalar(BO->getLHS());
        return true;
    }

    // New max/min reduction detection
    if (isMaxMinReduction(BO, LHSRef->getDecl())) {
        classifyScalar(BO->getLHS());
        return true;
    }

    return true;
}

        if (auto *ASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(LHS)) {
            if (containsSameArrayAccess(BO->getRHS(), ASE))
                classifyArray(BO->getLHS(), BO->getRHS());
        }
        return true;
    }

private:
    void classifyScalar(clang::Expr *LHS) {
        LHS = LHS->IgnoreParenImpCasts();
        if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(LHS))
            if (auto *VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl()))
                reductionVars.insert(VD);
    }

    void classifyArray(clang::Expr *LHS, clang::Expr * /*RHS*/) {
        auto *ASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(
            LHS->IgnoreParenImpCasts());
        if (!ASE || !innermostInductionVar) return;

        if (!subscriptDependsOn(ASE, innermostInductionVar)) {
            if (const clang::ValueDecl *Base = getBaseDecl(ASE))
                reductionVars.insert(Base);
        }
    }

    const clang::ValueDecl* getBaseDecl(clang::ArraySubscriptExpr *ASE) {
        clang::Expr *Base = ASE->getBase()->IgnoreParenImpCasts();
        if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(Base))
            return DRE->getDecl();
        if (auto *InnerASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(Base))
            return getBaseDecl(InnerASE);
        return nullptr;
    }

    bool subscriptDependsOn(clang::ArraySubscriptExpr *ASE,
                             const clang::VarDecl *IndVar) {
        struct Finder : clang::RecursiveASTVisitor<Finder> {
            const clang::VarDecl *Target; bool Found = false;
            bool VisitDeclRefExpr(clang::DeclRefExpr *R) {
                if (R->getDecl() == Target) Found = true;
                return true;
            }
        } F;
        F.Target = IndVar;

        clang::Expr *Base = ASE->getBase()->IgnoreParenImpCasts();
        F.TraverseStmt(ASE->getIdx());
        if (auto *InnerASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(Base))
            F.TraverseStmt(InnerASE->getIdx());
        return F.Found;
    }

    bool isMaxMinReduction(clang::BinaryOperator *Assign,
                           const clang::ValueDecl *ReductionVar)
    {
        if (!Context)
            return false;
        
        const clang::Stmt *Current = Assign;
    
        while (true)
        {
            auto Parents = Context->getParents(*Current);
    
            if (Parents.empty())
                break;
            
            if (const auto *If = Parents[0].get<clang::IfStmt>())
            {
                auto *Cond = llvm::dyn_cast<clang::BinaryOperator>(
                    If->getCond()->IgnoreParenImpCasts());
    
                if (!Cond || !Cond->isComparisonOp())
                    return false;
                
                return containsSameDeclRef(Cond->getLHS(), ReductionVar) ||
                       containsSameDeclRef(Cond->getRHS(), ReductionVar);
            }
    
            if (const auto *S = Parents[0].get<clang::Stmt>())
                Current = S;
            else
                break;
        }
    
        return false;
    }

    bool containsSameDeclRef(clang::Expr *E, const clang::ValueDecl *D) {
        struct Finder : clang::RecursiveASTVisitor<Finder> {
            const clang::ValueDecl *Target; bool Found = false;
            bool VisitDeclRefExpr(clang::DeclRefExpr *R) {
                if (R->getDecl() == Target) Found = true;
                return true;
            }
        } F;
        F.Target = D;
        F.TraverseStmt(E);
        return F.Found;
    }

    bool containsSameArrayAccess(clang::Expr *E, clang::ArraySubscriptExpr *LHSAse) {
        const clang::ValueDecl *LHSBase = getBaseDecl(LHSAse);
        if (!LHSBase) return false;

        struct Finder : clang::RecursiveASTVisitor<Finder> {
            const clang::ValueDecl *TargetBase; bool Found = false;
            ReductionVisitor *Self;
            bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr *ASE) {
                if (Self->getBaseDecl(ASE) == TargetBase) Found = true;
                return true;
            }
        } F;
        F.TargetBase = LHSBase;
        F.Self = this;
        F.TraverseStmt(E);
        return F.Found;
    }
};

void ASTFeatureExtractor::extractReductionVariables(
    LoopInfo &info,
    clang::ForStmt *FS,
    clang::ASTContext *Context)
{
    ReductionVisitor visitor;
    visitor.Context = Context;
    visitor.innermostInductionVar = getInductionVar(FS); // FS's own induction var, not the innermost nested loop
    visitor.TraverseStmt(FS->getBody());
    info.features.reductions = visitor.reductionVars.size();
}


long long ASTFeatureExtractor::getTripCount(
    clang::ForStmt *FS,
    clang::ASTContext *Context)
{
    auto *Init = FS->getInit();
    auto *Cond = FS->getCond();
    auto *Inc  = FS->getInc();

    if (!Init || !Cond || !Inc)
        return -1;

    auto *BO = llvm::dyn_cast<clang::BinaryOperator>(Cond);
    if (!BO || !BO->isComparisonOp())
        return -1;

    //------------------------------------------
    // Extract lower bound
    //------------------------------------------

    clang::Expr::EvalResult LBRes, UBRes;
    const clang::Expr *LBExpr = nullptr;

    if (auto *DS = llvm::dyn_cast<clang::DeclStmt>(Init))
    {
        if (DS->isSingleDecl())
            if (auto *VD = llvm::dyn_cast<clang::VarDecl>(DS->getSingleDecl()))
                LBExpr = VD->getInit();
    }
    else if (auto *InitBO = llvm::dyn_cast<clang::BinaryOperator>(Init))
    {
        if (InitBO->getOpcode() == clang::BO_Assign)
            LBExpr = InitBO->getRHS();
    }

    if (!LBExpr || !LBExpr->EvaluateAsInt(LBRes, *Context))
        return -1;

    //------------------------------------------
    // Extract upper bound
    //------------------------------------------

    const clang::Expr *UBExpr = BO->getRHS();

    if (!UBExpr->EvaluateAsInt(UBRes, *Context))
        return -1;

    //------------------------------------------
    // Determine step
    //------------------------------------------

    long long step = 0;

    if (auto *UO = llvm::dyn_cast<clang::UnaryOperator>(Inc))
    {
        if (UO->getOpcode() == clang::UO_PostInc ||
            UO->getOpcode() == clang::UO_PreInc)
        {
            step = 1;
        }
        else if (UO->getOpcode() == clang::UO_PostDec ||
                 UO->getOpcode() == clang::UO_PreDec)
        {
            step = -1;
        }
        else
        {
            return -1;
        }
    }
    else if (auto *CAO = llvm::dyn_cast<clang::CompoundAssignOperator>(Inc))
    {
        clang::Expr::EvalResult StepEval;

        if (!CAO->getRHS()->EvaluateAsInt(StepEval, *Context))
            return -1;

        long long amount = StepEval.Val.getInt().getExtValue();

        if (CAO->getOpcode() == clang::BO_AddAssign)
            step = amount;
        else if (CAO->getOpcode() == clang::BO_SubAssign)
            step = -amount;
        else
            return -1;
    }
    else
    {
        return -1;
    }

    if (step == 0)
        return -1;

    //------------------------------------------
    // Compute trip count
    //------------------------------------------

    long long lb = LBRes.Val.getInt().getExtValue();
    long long ub = UBRes.Val.getInt().getExtValue();

    long long trip = -1;

    if (step > 0)
    {
        switch (BO->getOpcode())
        {
        case clang::BO_LT:
            trip = (ub - lb + step - 1) / step;
            break;

        case clang::BO_LE:
            trip = (ub - lb) / step + 1;
            break;

        default:
            return -1;
        }
    }
    else
    {
        long long absStep = -step;

        switch (BO->getOpcode())
        {
        case clang::BO_GT:
            trip = (lb - ub + absStep - 1) / absStep;
            break;

        case clang::BO_GE:
            trip = (lb - ub) / absStep + 1;
            break;

        default:
            return -1;
        }
    }

    return (trip >= 0) ? trip : -1;
}

void ASTFeatureExtractor::extractIterationSpace(
    LoopInfo &info, clang::ForStmt *FS, clang::ASTContext *Context)
{
    // Per-loop trip count only — used for the diagnostic Loop Summary.
    // Region-level totals are computed separately via
    // computeNestedIterationSpace() once we know which loop is top-level.
    info.features.iterationSpace = getTripCount(FS, Context);
}

int ASTFeatureExtractor::computeMaxNestingDepth(clang::Stmt *S)
{
    if (!S) return 0;

    // Braceless bodies mean S itself can BE the nested loop
    // (e.g. `for(i...) for(j...) ...;`), not just contain one.
    if (auto *FS = llvm::dyn_cast<clang::ForStmt>(S))
        return 1 + computeMaxNestingDepth(FS->getBody());

    int maxChildDepth = 0;
    for (clang::Stmt *Child : S->children())
    {
        if (!Child) continue;
        maxChildDepth = std::max(maxChildDepth, computeMaxNestingDepth(Child));
    }
    return maxChildDepth;
}

long long ASTFeatureExtractor::computeNestedIterationSpace(
    clang::ForStmt *FS, clang::ASTContext *Context)
{
    long long trip = getTripCount(FS, Context);
    if (trip < 0) return -1;

    // Sum (not just descend into the first) all sibling for-loops in
    // FS's body -- e.g. gemm's i-loop contains a sibling j-loop AND a
    // sibling k-loop (itself nesting a j-loop); both run once per
    // outer iteration, so their spaces add rather than chain-multiply.
    long long childSpace = sumNestedIterationSpace(FS->getBody(), Context, getTripCount);
    if (childSpace < 0) return -1;

    return trip * (childSpace > 0 ? childSpace : 1);
}
