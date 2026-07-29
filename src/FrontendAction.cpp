#include "FrontendAction.h"

#include "ASTConsumer.h"
#include "PragmaCallbacks.h"

std::unique_ptr<clang::ASTConsumer>
FrontendAction::CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef File)
{
    regionDetector =
        std::make_unique<RegionDetector>(
            CI.getSourceManager());

    auto pragmaCallbacks =
        std::make_unique<PragmaCallbacks>(
            *regionDetector);

    CI.getPreprocessor().addPPCallbacks(
        std::move(pragmaCallbacks));

    return std::make_unique<ASTConsumer>(regionDetector.get(), outputDir, File.str());
}