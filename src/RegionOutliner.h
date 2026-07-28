// RegionOutliner.h
#ifndef REGION_OUTLINER_H
#define REGION_OUTLINER_H

#include <string>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include "ProfitabilityRegion.h"

class RegionOutliner
{
public:
    static std::string outlineRegion(
        ProfitabilityRegion &region,
        clang::FunctionDecl *EnclosingFD,
        clang::ASTContext *Context,
        const std::string &outputDir,
        unsigned regionIndex);
};

#endif