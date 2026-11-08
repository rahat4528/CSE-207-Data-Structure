#include "TopologicalSorter.h"
#include <algorithm>
#include <stack>

namespace DepResolver {

Result<InstallationPlan> TopologicalSorter::sortKahn(const DependencyGraph& graph) {
    size_t n = graph.nodeCount();
    if (n == 0) return Result<InstallationPlan>::ok(InstallationPlan{});

    // Count prerequisites for each package (number of forward dependencies)
    std::vector<size_t> prereqCount(n, 0);
    std::vector<size_t> level(n, 0);

    for (size_t u = 0; u < n; ++u) {
        prereqCount[u] = graph.getDependencies(static_cast<NodeId>(u)).size();
    }

    // Deterministic ready set: stores (packageName, NodeId) sorted ascending by name
    std::set<std::pair<std::string, NodeId>> readySet;
    for (size_t u = 0; u < n; ++u) {
        if (prereqCount[u] == 0) {
            readySet.emplace(graph.getNodeName(static_cast<NodeId>(u)), static_cast<NodeId>(u));
        }
    }

    InstallationPlan plan;
    size_t processedCount = 0;
    size_t maxLevel = 0;

    while (!readySet.empty()) {
        // Always pick lexicographically smallest ready package for 100% reproducible tie-break
        auto it = readySet.begin();
        std::string pkgName = it->first;
        NodeId u = it->second;
        readySet.erase(it);

        size_t uLevel = level[u];
        maxLevel = std::max(maxLevel, uLevel);

        InstallStep step;
        step.order = processedCount++;
        step.packageName = pkgName;
        step.level = uLevel;

        for (const auto& dep : graph.getDependencies(u)) {
            step.dependencies.push_back(graph.getNodeName(dep.target));
        }

        // Determine reason
        const auto& dependents = graph.getDependents(u);
        if (dependents.empty()) {
            step.reason = "Directly requested";
        } else {
            step.reason = "Required by " + graph.getNodeName(dependents.front().target);
        }

        plan.steps.push_back(std::move(step));

        // Notify all dependents that u is resolved
        for (const auto& revEdge : graph.getDependents(u)) {
            NodeId v = revEdge.target;
            level[v] = std::max(level[v], uLevel + 1);
            if (prereqCount[v] > 0) {
                prereqCount[v]--;
                if (prereqCount[v] == 0) {
                    readySet.emplace(graph.getNodeName(v), v);
                }
            }
        }
    }

    if (processedCount < n) {
        return Result<InstallationPlan>::err(
            ErrorCode::CYCLE_DETECTED,
            "Cannot generate topological installation order: circular dependency detected (Kahn count: " +
            std::to_string(processedCount) + " < " + std::to_string(n) + ")"
        );
    }

    plan.totalLevels = maxLevel + 1;
    return Result<InstallationPlan>::ok(plan);
}

Result<std::vector<std::string>> TopologicalSorter::sortDfs(const DependencyGraph& graph) {
    size_t n = graph.nodeCount();
    if (n == 0) return Result<std::vector<std::string>>::ok({});

    std::vector<uint8_t> visited(n, 0); // 0=unvisited, 1=in_progress, 2=done
    std::vector<std::string> postOrder;
    postOrder.reserve(n);

    // To produce valid installation order (prerequisites first), we traverse on reverse graph
    // (from prerequisites to dependents) or reverse the topological sort of forward graph
    bool hasCycle = false;

    auto dfs = [&](auto& self, NodeId u) -> void {
        visited[u] = 1;
        // Sort forward dependencies for determinism
        auto edges = graph.getDependencies(u);
        std::sort(edges.begin(), edges.end(), [&](const GraphEdge& a, const GraphEdge& b) {
            return graph.getNodeName(a.target) < graph.getNodeName(b.target);
        });

        for (const auto& edge : edges) {
            NodeId v = edge.target;
            if (visited[v] == 1) {
                hasCycle = true;
                return;
            }
            if (visited[v] == 0) {
                self(self, v);
                if (hasCycle) return;
            }
        }

        visited[u] = 2;
        postOrder.push_back(graph.getNodeName(u));
    };

    auto sortedNodes = graph.getSortedNodeIds();
    for (NodeId u : sortedNodes) {
        if (visited[u] == 0) {
            dfs(dfs, u);
            if (hasCycle) {
                return Result<std::vector<std::string>>::err(
                    ErrorCode::CYCLE_DETECTED,
                    "Cycle detected during DFS topological sort"
                );
            }
        }
    }

    // In postOrder, a node u finishes AFTER its dependencies have finished, so dependencies appear first!
    return Result<std::vector<std::string>>::ok(postOrder);
}

Result<std::vector<std::vector<std::string>>> TopologicalSorter::getParallelLayers(const DependencyGraph& graph) {
    auto planRes = sortKahn(graph);
    if (planRes.isErr()) return Result<std::vector<std::vector<std::string>>>::err(planRes.error());

    const auto& plan = planRes.value();
    std::vector<std::vector<std::string>> layers(plan.totalLevels);

    for (const auto& step : plan.steps) {
        layers[step.level].push_back(step.packageName);
    }

    // Sort each layer for determinism
    for (auto& layer : layers) {
        std::sort(layer.begin(), layer.end());
    }

    return Result<std::vector<std::vector<std::string>>>::ok(layers);
}

std::vector<std::vector<std::string>> TopologicalSorter::getMultipleOrders(const DependencyGraph& graph, size_t maxOrders) {
    size_t n = graph.nodeCount();
    if (n == 0) return {};

    std::vector<size_t> prereqCount(n, 0);
    for (size_t u = 0; u < n; ++u) {
        prereqCount[u] = graph.getDependencies(static_cast<NodeId>(u)).size();
    }

    std::vector<NodeId> currentOrder;
    std::vector<std::vector<std::string>> allOrders;
    std::vector<bool> used(n, false);

    auto backtrack = [&](auto& self) -> void {
        if (allOrders.size() >= maxOrders) return;

        if (currentOrder.size() == n) {
            std::vector<std::string> orderNames;
            for (NodeId id : currentOrder) {
                orderNames.push_back(graph.getNodeName(id));
            }
            allOrders.push_back(std::move(orderNames));
            return;
        }

        // Candidates: unused nodes with prereqCount == 0
        std::vector<NodeId> readyCandidates;
        for (size_t u = 0; u < n; ++u) {
            if (!used[u] && prereqCount[u] == 0) {
                readyCandidates.push_back(static_cast<NodeId>(u));
            }
        }
        std::sort(readyCandidates.begin(), readyCandidates.end(), [&](NodeId a, NodeId b) {
            return graph.getNodeName(a) < graph.getNodeName(b);
        });

        for (NodeId cand : readyCandidates) {
            used[cand] = true;
            currentOrder.push_back(cand);

            // Decrement dependents
            for (const auto& revEdge : graph.getDependents(cand)) {
                prereqCount[revEdge.target]--;
            }

            self(self);

            // Backtrack
            for (const auto& revEdge : graph.getDependents(cand)) {
                prereqCount[revEdge.target]++;
            }
            currentOrder.pop_back();
            used[cand] = false;

            if (allOrders.size() >= maxOrders) break;
        }
    };

    backtrack(backtrack);
    return allOrders;
}

} // namespace DepResolver

