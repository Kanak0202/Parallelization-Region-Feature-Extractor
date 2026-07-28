#ifndef AST_CONSUMER_H
#define AST_CONSUMER_H

#include <string>

#include <clang/AST/ASTConsumer.h>

#include "LoopManager.h"
#include "RegionDetector.h"
#include "CSVWriter.h"

class ASTConsumer : public clang::ASTConsumer
{
private:

    LoopManager manager;

    RegionDetector *regionDetector;

    std::string outputDir;

    std::string sourceFileName;

public:

    explicit ASTConsumer(
        RegionDetector *detector,
        std::string outputDir,
        std::string sourceFileName);

    void HandleTranslationUnit(
        clang::ASTContext &Context) override;
};

#endif