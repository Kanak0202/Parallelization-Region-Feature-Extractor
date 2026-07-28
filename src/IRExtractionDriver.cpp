// IRExtractionDriver.cpp
#include "IRExtractionDriver.h"
#include "IRFeaturePass.h"

#include <cstdlib>
#include <iostream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/TargetParser/Triple.h>

bool runIRExtraction(ProfitabilityRegion &region, const std::string &resourceDir)
{
    const std::string &srcPath = region.getOutlinedFilePath();
    if (srcPath.empty()) {
        std::cerr << "  [ir-extract] FAILED: region has no outlined file\n";
        return false;
    }

    std::string irPath = srcPath.substr(0, srcPath.size() - 2) + ".ll"; // strip .c

    std::string cmd = "clang -S -emit-llvm -O1 -ffp-contract=fast -fno-builtin " +
                   srcPath + " -o " + irPath +
                   " -resource-dir=" + resourceDir + " 2>&1";
                   
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "  [ir-extract] FAILED: clang compile of " << srcPath
                   << " returned " << rc << "\n";
        return false;
    }

    llvm::LLVMContext Context;
    llvm::SMDiagnostic Err;
    std::unique_ptr<llvm::Module> M = llvm::parseIRFile(irPath, Err, Context);
    if (!M) {
        std::cerr << "  [ir-extract] FAILED: could not parse " << irPath
                   << " -- " << Err.getMessage().str() << "\n";
        return false;
    }

    llvm::Function *F = M->getFunction(region.getOutlinedFunctionName());
    if (!F) {
        std::cerr << "  [ir-extract] FAILED: function '"
                   << region.getOutlinedFunctionName()
                   << "' not found in " << irPath << "\n";
        return false;
    }

    // Build just enough analysis infrastructure to get LoopInfo + SE
    // for this one function, without the full PassBuilder machinery.
    llvm::DominatorTree DT(*F);
    llvm::LoopInfo LI(DT);
    llvm::AssumptionCache AC(*F);
    llvm::Triple TT(M->getTargetTriple());
    llvm::TargetLibraryInfoImpl TLII(TT);
    llvm::TargetLibraryInfo TLI(TLII);
    llvm::ScalarEvolution SE(*F, TLI, AC, DT, LI);

    auto &loops = region.getLoops();   // add the & -- this was the whole bug
if (loops.empty()) {
    std::cerr << "  [ir-extract] FAILED: region has no recorded loop to merge into\n";
    return false;
}

    // Region invariant established earlier: exactly one top-level loop
    // per region carries the authoritative FeatureVector.
    extractIRFeatures(*F, SE, LI, loops[0].features);

    std::cout << "  [ir-extract] OK: merged IR features from " << irPath << "\n";
    return true;
}