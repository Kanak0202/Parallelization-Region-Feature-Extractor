#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <string>
#include <vector>

#include <clang/AST/RecursiveASTVisitor.h>

#include "LoopManager.h"
#include "RegionDetector.h"
#include "RegionOutliner.h"

class ASTVisitor : public clang::RecursiveASTVisitor<ASTVisitor>
{
private:

    clang::ASTContext *Context;

    LoopManager &manager;

    RegionDetector *regionDetector;

    std::string outputDir;
    unsigned regionCounter = 0;

    std::vector<clang::ForStmt*> loopStack;

public:

    explicit ASTVisitor(
        clang::ASTContext *Context,
        LoopManager &manager,
        RegionDetector *regionDetector,
        std::string outputDir);

    bool VisitFunctionDecl(clang::FunctionDecl *FD);

    bool VisitForStmt(clang::ForStmt *FS);

    bool TraverseForStmt(clang::ForStmt *FS);
};

#endif