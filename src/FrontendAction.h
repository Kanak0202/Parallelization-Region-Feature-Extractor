#ifndef FRONTEND_ACTION_H
#define FRONTEND_ACTION_H

#include <memory>
#include <string>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>

#include "RegionDetector.h"

class FrontendAction : public clang::ASTFrontendAction
{
private:

    std::unique_ptr<RegionDetector> regionDetector;
    std::string outputDir;

public:

    explicit FrontendAction(std::string outputDir = "extracted")
        : outputDir(std::move(outputDir))
    {
    }

    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(
        clang::CompilerInstance &CI,
        llvm::StringRef File) override;


  RegionDetector* getRegionDetector(){
return regionDetector.get();

}

};

#endif