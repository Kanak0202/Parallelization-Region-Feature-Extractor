// CSVWriter.cpp
#include "CSVWriter.h"
#include <fstream>
#include <sys/stat.h>

namespace {
bool fileExists(const std::string &path)
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}
}

void appendRegionToCSV(const std::string &csvPath,
                        unsigned regionId,
                        const std::string &fileName,
                        const FeatureVector &f)
{
    bool needHeader = !fileExists(csvPath);

    std::ofstream ofs(csvPath, std::ios::app);
    if (!ofs) return;

    if (needHeader)
    {
        ofs << "RegionID,FileName,LoopDepth,IterationSpace,BasicBlocks,"
               "IntArithmetic,FloatArithmetic,IntMultiply,FloatMultiply,"
               "IntDivision,FloatDivision,SpecialFunctions,FMAOperations,"
               "Loads,Stores,BytesRead,BytesWritten,StrideClass,"
               "IndirectAccesses,ReductionVars,FunctionCalls,ArraysAccessed,BranchCount,"
               "SerialTime,OpenMP3Time,"
               "OpenMP45ResidentTime,OpenMP45ObservedTime,OpenMP45IsolatedTime,"
               "OpenACCResidentTime,OpenACCObservedTime,OpenACCIsolatedTime\n";
    }

    ofs << regionId << ",\"" << fileName << "\","
        << f.loopDepth << "," << f.iterationSpace << "," << f.basicBlocks << ","
        << f.intArithmetic << "," << f.floatArithmetic << ","
        << f.intMultiply << "," << f.floatMultiply << ","
        << f.intDivision << "," << f.floatDivision << ","
        << f.specialFunctions << "," << f.fmaOperations << ","
        << f.loads << "," << f.stores << ","
        << f.bytesRead << "," << f.bytesWritten << ","
        << f.strideClass << "," << f.indirectAccesses << ","
        << f.reductions << "," << f.functionCalls << ","
        << f.arraysAccessed << "," << f.branchCount << ",,,,,,,," << "\n";
}