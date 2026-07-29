#include "LoopInfo.h"

#include <iostream>

void LoopInfo::print() const
{
    std::cout << "---------------------------------\n";

    std::cout << "Loop\n";

    std::cout << "Line Number       : "
              << lineNumber << '\n';

    std::cout << "Column Number     : "
              << columnNumber << '\n';


    std::cout << "Nesting Depth : "
          << features.loopDepth
          << std::endl; 
    std::cout << "Iteration Space : "
          << features.iterationSpace
          << std::endl; 
    std::cout << "Function Calls : "
          << features.functionCalls
          << std::endl; 
    std::cout << "Arrays Accessed : "
          << features.arraysAccessed
          << std::endl; 
    std::cout << "Reduction Vars : "
          << features.reductions
          << std::endl; 
    std::cout << "Indirect Accesses : "
          << features.indirectAccesses
          << std::endl;

      std::cout << "Basic Blocks      : " << features.basicBlocks << std::endl;
std::cout << "Int Arithmetic    : " << features.intArithmetic << std::endl;
std::cout << "Float Arithmetic  : " << features.floatArithmetic << std::endl;
std::cout << "Int Multiply      : " << features.intMultiply << std::endl;
std::cout << "Float Multiply    : " << features.floatMultiply << std::endl;
std::cout << "Int Division      : " << features.intDivision << std::endl;
std::cout << "Float Division    : " << features.floatDivision << std::endl;
std::cout << "Special Functions : " << features.specialFunctions << std::endl;
std::cout << "FMA Operations    : " << features.fmaOperations << std::endl;
std::cout << "Loads             : " << features.loads << std::endl;
std::cout << "Stores            : " << features.stores << std::endl;
std::cout << "Bytes Read        : " << features.bytesRead << std::endl;
std::cout << "Bytes Written     : " << features.bytesWritten << std::endl;
std::cout << "Stride Class      : " << features.strideClass << std::endl;
std::cout << "Branch Count      : " << features.branchCount << std::endl;
}
