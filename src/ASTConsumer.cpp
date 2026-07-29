#include "ASTConsumer.h"
#include "ASTVisitor.h"
#include "IRExtractionDriver.h"
#include "CSVWriter.h"

ASTConsumer::ASTConsumer(
    RegionDetector *detector,
    std::string outputDir,
    std::string sourceFileName)
    : regionDetector(detector), outputDir(std::move(outputDir)), sourceFileName(std::move(sourceFileName))
{
}

void ASTConsumer::HandleTranslationUnit(
    clang::ASTContext &Context)
{
    ASTVisitor Visitor(
        &Context,
        manager,
        regionDetector,
        outputDir);

    Visitor.TraverseDecl(
        Context.getTranslationUnitDecl());

    if (regionDetector)
    {
        for (auto &region : regionDetector->getRegions())
        {
            runIRExtraction(region, /* resourceDir */ "/home/capc1/miniforge3/lib/clang/22");
            auto &loops = region.getLoops();    
            if (!loops.empty())
                appendRegionToCSV("features.csv", region.getRegionId(),
                                   sourceFileName, loops[0].features);    
        }
        regionDetector->printRegions();
    }
    // manager.printAll();

    // if (regionDetector)
    // {
    //     regionDetector->printRegions();
    // }
}