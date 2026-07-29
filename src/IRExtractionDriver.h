// IRExtractionDriver.h
#ifndef IR_EXTRACTION_DRIVER_H
#define IR_EXTRACTION_DRIVER_H

#include "ProfitabilityRegion.h"

// Compiles region.getOutlinedFilePath() to LLVM IR via clang -O1,
// parses it back in, locates region.getOutlinedFunctionName(), runs
// the feature pass, and merges results into the region's stored
// FeatureVector (its single top-level LoopInfo entry).
bool runIRExtraction(ProfitabilityRegion &region, const std::string &resourceDir);

#endif