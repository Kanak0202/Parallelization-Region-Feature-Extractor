// LoopUtils.cpp
#include "LoopUtils.h"
#include <clang/AST/Expr.h>

clang::ForStmt* findDirectlyNestedFor(clang::Stmt *S)
{
    if (!S) return nullptr;
    if (auto *FS = llvm::dyn_cast<clang::ForStmt>(S))
        return FS;
    for (clang::Stmt *Child : S->children())
        if (auto *FS = llvm::dyn_cast_or_null<clang::ForStmt>(Child))
            return FS;
    return nullptr;
}

clang::VarDecl* getInductionVar(clang::ForStmt *FS)
{
    if (!FS || !FS->getInit()) return nullptr;
    if (auto *DS = llvm::dyn_cast<clang::DeclStmt>(FS->getInit()))
        if (DS->isSingleDecl())
            return llvm::dyn_cast<clang::VarDecl>(DS->getSingleDecl());
    if (auto *BO = llvm::dyn_cast<clang::BinaryOperator>(FS->getInit()))
        if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(BO->getLHS()))
            return llvm::dyn_cast<clang::VarDecl>(DRE->getDecl());
    return nullptr;
}

// Trip count of FS itself, multiplied by whatever's nested inside it
// (1 if FS's body has no further nested for-loop).
static long long iterationSpaceOfLoop(clang::ForStmt *FS, clang::ASTContext *Context,
                                       const TripCountFn &getTripCount)
{
    long long trip = getTripCount(FS, Context);
    if (trip < 0) return -1;

    long long childSpace = sumNestedIterationSpace(FS->getBody(), Context, getTripCount);
    if (childSpace < 0) return -1;

    return trip * (childSpace > 0 ? childSpace : 1);
}

long long sumNestedIterationSpace(
    clang::Stmt *S,
    clang::ASTContext *Context,
    const TripCountFn &getTripCount)
{
    if (!S)
        return 0;

    long long total = 0;

    if (auto *FS = llvm::dyn_cast<clang::ForStmt>(S))
    {
        long long sub = iterationSpaceOfLoop(FS, Context, getTripCount);

        if (sub < 0)
            return -1;

        total += sub;

        // Don't recurse into this loop because
        // iterationSpaceOfLoop() already accounts for
        // all loops nested inside it.
        return total;
    }

    for (clang::Stmt *Child : S->children())
    {
        if (!Child)
            continue;

        long long sub = sumNestedIterationSpace(
            Child,
            Context,
            getTripCount);

        if (sub < 0)
            return -1;

        total += sub;
    }

    return total;
}