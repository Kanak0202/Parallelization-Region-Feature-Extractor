// IRFeaturePass.h
#ifndef IR_FEATURE_PASS_H
#define IR_FEATURE_PASS_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/LoopInfo.h>
#include "FeatureVector.h"
#include <cstdint>

// strideClass encoding — stored as the MAX class seen across all
// memory ops in the region, since one bad access pattern (indirect)
// dominates the profitability story even if everything else is unit-stride.
enum StrideClass : int {
    STRIDE_NONE      = 0, // no SCEV-classifiable access seen yet
    STRIDE_UNIT      = 1, // constant stride == element size (contiguous)
    STRIDE_CONSTANT  = 2, // constant stride != element size (e.g. row skip)
    STRIDE_VARIABLE  = 3  // loop-variant but not affine, and not indirect
};

void extractIRFeatures(llvm::Function &F,
                        llvm::ScalarEvolution &SE,
                        llvm::LoopInfo &LI,
                        FeatureVector &FV);

void classifyAccess(llvm::Value *Ptr,
                     llvm::ScalarEvolution &SE,
                     llvm::LoopInfo &LI,
                     uint64_t elemSizeBytes,
                     llvm::SmallPtrSetImpl<llvm::Value*> &classifiedIndirectPtrs,
                     FeatureVector &FV);

#endif