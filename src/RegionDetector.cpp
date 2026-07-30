#include "RegionDetector.h"

#include <iostream>
#include <sstream>

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

void RegionDetector::recordMacro(clang::SourceLocation Loc)
{
    if (Loc.isInvalid())
        return;

    clang::SourceLocation SpellLoc = SM.getSpellingLoc(Loc);
    if (SpellLoc.isInvalid())
        return;

    clang::FileID FID = SM.getFileID(SpellLoc);
    unsigned startLine = SM.getSpellingLineNumber(SpellLoc);

    bool invalid = false;
    llvm::StringRef Buf = SM.getBufferData(FID, &invalid);
    if (invalid)
        return;

    // A macro's #define can span multiple physical lines via
    // backslash-continuation (e.g. POLYBENCH_ALLOC_2D_ARRAY). Grabbing
    // only the first line truncates it mid-continuation, leaving a
    // dangling trailing '\' that splices onto whatever text follows
    // it in the generated preamble -- which then swallows the next
    // macro's leading '#' as a stringize operator. Walk forward and
    // capture every continuation line too.
    std::istringstream stream(Buf.str());
    std::string line, text;
    unsigned lineNo = 0;
    bool capturing = false;

    while (std::getline(stream, line))
    {
        ++lineNo;
        if (!capturing)
        {
            if (lineNo != startLine)
                continue;
            capturing = true;
        }

        if (!text.empty())
            text += "\n";
        text += line;

        std::string trimmed = line;
        while (!trimmed.empty() && trimmed.back() == '\r')
            trimmed.pop_back();

        if (trimmed.empty() || trimmed.back() != '\\')
            break; // no continuation -- definition ends here

        // else: loop continues onto the next physical line
    }

    if (!capturing)
        return;

    macros.push_back({text, startLine, FID});
}

std::string RegionDetector::getMacroPreamble(clang::FileID FID, unsigned beforeLine) const
{
    std::string out;
    for (const auto &m : macros)
    {
        if (m.fileID == FID && m.line < beforeLine)
            out += m.text + "\n";
    }
    return out;
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
