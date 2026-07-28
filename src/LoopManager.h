#ifndef LOOP_MANAGER_H
#define LOOP_MANAGER_H

#include <vector>

#include "LoopInfo.h"

class LoopManager
{
private:
    std::vector<LoopInfo> loops;

public:
    void addLoop(const LoopInfo& info);

    std::vector<LoopInfo>& getLoops();

    void printAll() const;
};

#endif
