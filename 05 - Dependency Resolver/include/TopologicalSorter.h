#pragma once

#include "Common.h"
#include "DependencyGraph.h"
#include <string>
#include <vector>
#include <set>
#include <queue>

namespace DepResolver {

struct InstallStep {
    size_t order{0};
    std::string packageName;
    std::string version;
    size_t level{0}; // Parallel installation stage (E4)
    std::string reason; // e.g. "Directly requested", "Required by web_app"
    std::vector<std::string> dependencies;
};

struct InstallationPlan {
    std::vector<InstallStep> steps;
    size_t totalLevels{0};
    bool isReproducible{true};

    std::vector<std::string> getPackageOrder() const {
        std::vector<std::string> order;
        order.reserve(steps.size());
        for (const auto& step : steps) {
            order.push_back(step.packageName);
        }
        return order;
    }
};

class TopologicalSorter {
public:
    // Primary: Deterministic Kahn's Algorithm (BFS) with Min-Priority Queue
    // O(V + E) or O((V + E) log V) with priority tie-breaking
    static Result<InstallationPlan> sortKahn(const DependencyGraph& graph);

    // Secondary: DFS Post-Order Reversal Topological Sort
    static Result<std::vector<std::string>> sortDfs(const DependencyGraph& graph);

    // Parallel Execution Layers (E4): Groups packages into concurrent batches
    static Result<std::vector<std::vector<std::string>>> getParallelLayers(const DependencyGraph& graph);

    // Multiple Valid Orders (A3): Backtracking generation of up to K distinct valid topological orders
    static std::vector<std::vector<std::string>> getMultipleOrders(const DependencyGraph& graph, size_t maxOrders = 10);
};

} // namespace DepResolver

