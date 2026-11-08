#pragma once

#include "Common.h"
#include "DependencyGraph.h"
#include <string>
#include <vector>
#include <optional>

namespace DepResolver {

struct CycleResult {
    bool hasCycle{false};
    std::vector<std::string> cyclePath; // e.g. ["A", "B", "C", "A"]
    std::string message;

    std::string pathToString() const {
        if (cyclePath.empty()) return "";
        std::string res;
        for (size_t i = 0; i < cyclePath.size(); ++i) {
            res += cyclePath[i];
            if (i + 1 < cyclePath.size()) res += " -> ";
        }
        return res;
    }
};

class CycleDetector {
public:
    // Iterative 3-State DFS Cycle Detection with Explicit Stack (Never overflows call stack)
    // Guarantees deterministic cycle detection: roots and neighbors visited in sorted order
    static CycleResult detectCycle(const DependencyGraph& graph);

    // Dynamic O(V + E) edge check: returns true if adding u -> v creates a cycle (i.e. v can reach u)
    static bool willCreateCycle(const DependencyGraph& graph, const std::string& fromPkg, const std::string& toPkg);

    // Tarjan's Strongly Connected Components (SCC) algorithm (E3)
    // Returns all cyclic groups of packages
    static std::vector<std::vector<std::string>> findCyclicGroups(const DependencyGraph& graph);
};

} // namespace DepResolver

