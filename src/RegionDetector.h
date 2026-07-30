#ifndef REGION_DETECTOR_H
#define REGION_DETECTOR_H

#include <vector>
#include <string>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include "ProfitabilityRegion.h"

struct MacroRecord
{
    std::string text;
    unsigned line;
    clang::FileID fileID;
};

class RegionDetector
{
private:

    clang::SourceManager &SM;

    std::vector<ProfitabilityRegion> regions;

    std::vector<MacroRecord> macros;

    unsigned nextRegionId = 0;

    bool insideRegion;

    ProfitabilityRegion currentRegion;

public:

    explicit RegionDetector(clang::SourceManager &SM);

    void handlePragma(clang::SourceLocation Loc);

    void recordMacro(clang::SourceLocation Loc);

    std::string getMacroPreamble(clang::FileID FID, unsigned beforeLine) const;
    
    ProfitabilityRegion* findRegion(unsigned lineNumber);

    std::vector<ProfitabilityRegion>& getRegions();

    const std::vector<ProfitabilityRegion>& getRegions() const;

    void printRegions() const;
};

#endif
