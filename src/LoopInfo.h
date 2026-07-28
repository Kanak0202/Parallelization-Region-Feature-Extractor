#ifndef LOOP_INFO_H
#define LOOP_INFO_H

#include <string>
#include "FeatureVector.h"

struct LoopInfo
{
    // Basic information
    unsigned lineNumber = 0;
    unsigned columnNumber = 0;

    // AST features
    FeatureVector features;

    void print() const;
};

#endif
