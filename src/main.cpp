#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>

#include "FrontendAction.h"

using namespace clang::tooling;

static llvm::cl::OptionCategory
ProfitabilityCategory("Profitability Tool Options");

static llvm::cl::opt<std::string> OutputDir(
    "output-dir",
    llvm::cl::desc("Directory to write outlined region .c files"),
    llvm::cl::init("extracted"),
    llvm::cl::cat(ProfitabilityCategory));

namespace {
class ProfitabilityActionFactory : public FrontendActionFactory
{
public:
    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<FrontendAction>(OutputDir);
    }
};
}

int main(int argc, const char **argv)
{
    auto ExpectedParser =
        CommonOptionsParser::create(
            argc,
            argv,
            ProfitabilityCategory);

    if (!ExpectedParser)
    {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }

    CommonOptionsParser &OptionsParser =
        ExpectedParser.get();

    llvm::sys::fs::create_directories(OutputDir);

    ClangTool Tool(
        OptionsParser.getCompilations(),
        OptionsParser.getSourcePathList());

    ProfitabilityActionFactory Factory;
    return Tool.run(&Factory);
}