#ifndef AST_FEATURE_EXTRACTOR_H
#define AST_FEATURE_EXTRACTOR_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>

#include "LoopInfo.h"

class ASTFeatureExtractor
{
public:
    static void extractFeatures(LoopInfo &info,
                        clang::ForStmt *FS,
                        clang::ASTContext *Context);

    static long long getTripCount(clang::ForStmt *FS, clang::ASTContext *Context);
    static int computeMaxNestingDepth(clang::Stmt *S);
    static long long computeNestedIterationSpace(clang::ForStmt *FS, clang::ASTContext *Context);

private:

    static void extractLocation(
        LoopInfo &info,
        clang::ForStmt *FS,
        clang::ASTContext *Context);

    static void extractLoopDepth(
        LoopInfo &info,
        clang::ForStmt *FS,
        clang::ASTContext *Context);

    static void extractIterationSpace(
        LoopInfo &info,
        clang::ForStmt *FS,
        clang::ASTContext *Context);

    static void extractFunctionCalls(
        LoopInfo &info,
        clang::ForStmt *FS);

    static void extractArraysAccessed(
        LoopInfo &info,
        clang::ForStmt *FS);

    static void extractReductionVariables(
        LoopInfo &info,
        clang::ForStmt *FS,
        clang::ASTContext *Context);


};

#endif
