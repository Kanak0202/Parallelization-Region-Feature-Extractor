#ifndef FEATURE_VECTOR_H
#define FEATURE_VECTOR_H

#include <string>

struct FeatureVector
{
    int loopDepth = 0;

    long long iterationSpace = -1;

    int basicBlocks = 0;

    int intArithmetic = 0;
    int floatArithmetic = 0;

    int intMultiply = 0;
    int floatMultiply = 0;

    int intDivision = 0;
    int floatDivision = 0;

    int specialFunctions = 0;
    int fmaOperations = 0;

    int loads = 0;
    int stores = 0;

    long long bytesRead = 0;
    long long bytesWritten = 0;

    int strideClass = 0;

    int indirectAccesses = 0;

    int reductions = 0;

    int functionCalls = 0;

    int arraysAccessed = 0;

    int branchCount = 0;
};

#endif
