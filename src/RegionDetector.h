#ifndef REGION_DETECTOR_H
#define REGION_DETECTOR_H

#include <vector>
#include <string>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include "ProfitabilityRegion.h"

class RegionDetector
{
private:

    clang::SourceManager &SM;

    std::vector<ProfitabilityRegion> regions;

    unsigned nextRegionId = 0;

    bool insideRegion;

    ProfitabilityRegion currentRegion;

public:

    explicit RegionDetector(clang::SourceManager &SM);

    void handlePragma(clang::SourceLocation Loc);
    
    ProfitabilityRegion* findRegion(unsigned lineNumber);

    std::vector<ProfitabilityRegion>& getRegions();

    const std::vector<ProfitabilityRegion>& getRegions() const;

    void printRegions() const;
};

#endif
