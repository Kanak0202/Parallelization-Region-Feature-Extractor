#ifndef PROFITABILITY_REGION_H
#define PROFITABILITY_REGION_H

#include <vector>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include "LoopInfo.h"

class ProfitabilityRegion
{
private:

    clang::SourceLocation beginLoc;
    clang::SourceLocation endLoc;

    std::vector<LoopInfo> loops;

    std::string outlinedFilePath;
    std::string outlinedFunctionName;

    unsigned regionId = 0;

public:

    void setBegin(clang::SourceLocation loc);
    void setEnd(clang::SourceLocation loc);

    clang::SourceLocation getBegin() const;
    clang::SourceLocation getEnd() const;

    void addLoop(const LoopInfo &loop);

    void setOutlinedInfo(const std::string &filePath, const std::string &funcName);
    const std::string& getOutlinedFilePath() const;
    const std::string& getOutlinedFunctionName() const;
    
    unsigned getBeginLine(const clang::SourceManager &SM) const;
    unsigned getEndLine(const clang::SourceManager &SM) const;

    void setRegionId(unsigned id);
    unsigned getRegionId() const;

    std::vector<LoopInfo>& getLoops();

    void print() const;
};

#endif
