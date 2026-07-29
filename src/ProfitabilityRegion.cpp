#include <clang/Basic/SourceManager.h>
#include "ProfitabilityRegion.h"

#include <iostream>

void ProfitabilityRegion::setBegin(clang::SourceLocation loc)
{
    beginLoc = loc;
}

void ProfitabilityRegion::setEnd(clang::SourceLocation loc)
{
    endLoc = loc;
}

clang::SourceLocation
ProfitabilityRegion::getBegin() const
{
    return beginLoc;
}

clang::SourceLocation
ProfitabilityRegion::getEnd() const
{
    return endLoc;
}

unsigned ProfitabilityRegion::getBeginLine(const clang::SourceManager &SM) const{
 return SM.getSpellingLineNumber(beginLoc);
}

unsigned ProfitabilityRegion::getEndLine(const clang::SourceManager &SM) const{
 return SM.getSpellingLineNumber(endLoc);
}


void ProfitabilityRegion::addLoop(const LoopInfo &loop)
{
    loops.push_back(loop);
}

std::vector<LoopInfo>&
ProfitabilityRegion::getLoops()
{
    return loops;
}

void ProfitabilityRegion::print() const
{
    std::cout << "\n========== Region ==========\n";

    std::cout << "Loops : "
              << loops.size()
              << "\n\n";

    for(const auto &loop : loops)
    {
        loop.print();
    }
}

void ProfitabilityRegion::setOutlinedInfo(
    const std::string &filePath, const std::string &funcName)
{
    outlinedFilePath = filePath;
    outlinedFunctionName = funcName;
}

const std::string& ProfitabilityRegion::getOutlinedFilePath() const
{
    return outlinedFilePath;
}

const std::string& ProfitabilityRegion::getOutlinedFunctionName() const
{
    return outlinedFunctionName;
}

void ProfitabilityRegion::setRegionId(unsigned id)
{
    regionId = id;
}

unsigned ProfitabilityRegion::getRegionId() const
{
    return regionId;
}