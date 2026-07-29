#include "RegionDetector.h"

#include <iostream>

RegionDetector::RegionDetector(clang::SourceManager &SM)
    : SM(SM),
      insideRegion(false)
{
}

void RegionDetector::handlePragma(clang::SourceLocation Loc)
{
    bool Invalid = false;

    const char *Buffer = SM.getCharacterData(Loc, &Invalid);

    if (Invalid)
        return;

    std::string Line;

    while (*Buffer &&
           *Buffer != '\n' &&
           *Buffer != '\r')
    {
        Line += *Buffer;
        ++Buffer;
    }

    // Ignore all non-CAPC pragmas
    if (Line.find("capc") == std::string::npos)
        return;

    if (Line.find("profitability_region") == std::string::npos)
        return;

    unsigned lineNo = SM.getSpellingLineNumber(Loc);

    if (Line.find("begin") != std::string::npos)
    {
        std::cout << "CAPC BEGIN detected at line "
                  << lineNo
                  << std::endl;

        currentRegion = ProfitabilityRegion();

        currentRegion.setBegin(Loc);
        currentRegion.setRegionId(nextRegionId++);
        insideRegion = true;
    }
    else if (Line.find("end") != std::string::npos)
    {
        std::cout << "CAPC END detected at line "
                  << lineNo
                  << std::endl;

        if (insideRegion)
        {
            currentRegion.setEnd(Loc);

            regions.push_back(currentRegion);

            insideRegion = false;
        }
        else
        {
            std::cout
                << "Warning: END encountered without matching BEGIN."
                << std::endl;
        }
    }
}

std::vector<ProfitabilityRegion>&
RegionDetector::getRegions()
{
    return regions;
}


const std::vector<ProfitabilityRegion>&
RegionDetector::getRegions() const
{
    return regions;
}

void RegionDetector::printRegions() const
{
    std::cout << "\n========== Regions Summary ==========\n";

    std::cout << "Number of Regions : "
              << regions.size()
              << std::endl;

    for (size_t i = 0; i < regions.size(); ++i)
    {
        unsigned beginLine =
            SM.getSpellingLineNumber(
                regions[i].getBegin());

        unsigned endLine =
            SM.getSpellingLineNumber(
                regions[i].getEnd());

        std::cout << "\nRegion "
                  << i + 1
                  << std::endl;
        
        std::cout << "Region ID : " << regions[i].getRegionId() << std::endl;

        std::cout << "Begin : "
                  << beginLine
                  << std::endl;

        std::cout << "End   : "
                  << endLine
                  << std::endl;

	regions[i].print();

    }
}

ProfitabilityRegion* RegionDetector::findRegion(
    unsigned lineNumber) {

std::cout << "Searching for loop at line: "
          << lineNumber
          << std::endl;

    for (auto &region : regions)
    {
        unsigned begin =
            region.getBeginLine(SM);
        unsigned end =
            region.getEndLine(SM);

	std::cout << "Checking region ["<<begin<<","<<end<<"]\n";
        if (lineNumber >= begin &&
            lineNumber <= end)
        {

	    std::cout<<"Match found!\n";
            return &region;
        }
    }

    std::cout<<"No matching region.\n";
    return nullptr;
}
