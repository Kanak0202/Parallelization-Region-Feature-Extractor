#include "LoopManager.h"

#include <iostream>

void LoopManager::addLoop(const LoopInfo& info)
{
    loops.push_back(info);
}

std::vector<LoopInfo>& LoopManager::getLoops()
{
    return loops;
}

void LoopManager::printAll() const
{
    std::cout << "\n========== Loop Summary ==========\n";
    std::cout << "Number of loops: "
              << loops.size()
              << "\n\n";

    for (const auto& loop : loops)
    {
        loop.print();
    }
}
