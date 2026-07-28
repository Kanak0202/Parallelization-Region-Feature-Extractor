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