#include "CycleDetector.h"
#include <stack>
#include <algorithm>

namespace DepResolver {

namespace {

enum class NodeColor : uint8_t {
    WHITE = 0, // Unvisited
    GRAY = 1,  // In progress (on current DFS call stack)
    BLACK = 2  // Completed
};

struct DfsFrame {
    NodeId u;
    size_t edgeIdx;
    std::vector<NodeId> sortedNeighbors;
};

} // namespace

CycleResult CycleDetector::detectCycle(const DependencyGraph& graph) {
    size_t n = graph.nodeCount();
    if (n == 0) return {false, {}, "Graph is empty"};

    std::vector<NodeColor> color(n, NodeColor::WHITE);
    std::vector<NodeId> currentPath; // Nodes currently colored GRAY in order
    currentPath.reserve(n);

    // Process all nodes in sorted order for strict determinism
    std::vector<NodeId> sortedNodes = graph.getSortedNodeIds();

    for (NodeId root : sortedNodes) {
        if (color[root] != NodeColor::WHITE) continue;

        std::vector<DfsFrame> stack;
        stack.reserve(n);

        // Prepare sorted neighbors for root
        auto getNeighbors = [&](NodeId u) {
            std::vector<NodeId> neighbors;
            for (const auto& edge : graph.getDependencies(u)) {
                neighbors.push_back(edge.target);
            }
            std::sort(neighbors.begin(), neighbors.end(), [&](NodeId a, NodeId b) {
                return graph.getNodeName(a) < graph.getNodeName(b);
            });
            return neighbors;
        };

        color[root] = NodeColor::GRAY;
        currentPath.push_back(root);
        stack.push_back({root, 0, getNeighbors(root)});

        while (!stack.empty()) {
            DfsFrame& top = stack.back();

            if (top.edgeIdx < top.sortedNeighbors.size()) {
                NodeId v = top.sortedNeighbors[top.edgeIdx++];

                if (color[v] == NodeColor::GRAY) {
                    // Circular dependency detected!
                    // Extract exact cycle from currentPath: from first occurrence of v to current top, then append v
                    std::vector<std::string> path;
                    auto it = std::find(currentPath.begin(), currentPath.end(), v);
                    if (it != currentPath.end()) {
                        for (; it != currentPath.end(); ++it) {
                            path.push_back(graph.getNodeName(*it));
                        }
                    } else {
                        path.push_back(graph.getNodeName(v));
                    }
                    path.push_back(graph.getNodeName(v)); // Close the cycle loop

                    CycleResult res;
                    res.hasCycle = true;
                    res.cyclePath = std::move(path);
                    res.message = "Circular dependency detected: " + res.pathToString();
                    return res;
                }

                if (color[v] == NodeColor::WHITE) {
                    color[v] = NodeColor::GRAY;
                    currentPath.push_back(v);
                    stack.push_back({v, 0, getNeighbors(v)});
                }
            } else {
                // Done with all neighbors of top.u
                color[top.u] = NodeColor::BLACK;
                currentPath.pop_back();
                stack.pop_back();
            }
        }
    }

    return {false, {}, "No circular dependencies found"};
}

bool CycleDetector::willCreateCycle(const DependencyGraph& graph, const std::string& fromPkg, const std::string& toPkg) {
    if (fromPkg == toPkg) return true; // Self loop

    auto uOpt = graph.findNodeId(fromPkg);
    auto vOpt = graph.findNodeId(toPkg);
    if (!uOpt.has_value() || !vOpt.has_value()) {
        return false;
    }

    NodeId u = uOpt.value();
    NodeId v = vOpt.value();

    // Adding u -> v creates a cycle iff v can already reach u
    size_t n = graph.nodeCount();
    std::vector<bool> visited(n, false);
    std::vector<NodeId> stack;
    stack.push_back(v);
    visited[v] = true;

    while (!stack.empty()) {
        NodeId curr = stack.back();
        stack.pop_back();

        if (curr == u) return true; // Path v -> ... -> u exists!

        for (const auto& edge : graph.getDependencies(curr)) {
            if (!visited[edge.target]) {
                visited[edge.target] = true;
                stack.push_back(edge.target);
            }
        }
    }

    return false;
}

std::vector<std::vector<std::string>> CycleDetector::findCyclicGroups(const DependencyGraph& graph) {
    size_t n = graph.nodeCount();
    std::vector<int> dfn(n, -1);
    std::vector<int> low(n, -1);
    std::vector<bool> inStack(n, false);
    std::vector<NodeId> st;
    int timer = 0;

    std::vector<std::vector<std::string>> sccList;

    auto tarjanDfs = [&](auto& self, NodeId u) -> void {
        dfn[u] = low[u] = ++timer;
        st.push_back(u);
        inStack[u] = true;

        for (const auto& edge : graph.getDependencies(u)) {
            NodeId v = edge.target;
            if (dfn[v] == -1) {
                self(self, v);
                low[u] = std::min(low[u], low[v]);
            } else if (inStack[v]) {
                low[u] = std::min(low[u], dfn[v]);
            }
        }

        if (dfn[u] == low[u]) {
            std::vector<std::string> comp;
            while (true) {
                NodeId node = st.back();
                st.pop_back();
                inStack[node] = false;
                comp.push_back(graph.getNodeName(node));
                if (node == u) break;
            }
            // Check if this SCC represents a cycle (size > 1 or single node with self-loop)
            if (comp.size() > 1 || (comp.size() == 1 && graph.hasSelfDependency(u))) {
                std::sort(comp.begin(), comp.end());
                sccList.push_back(std::move(comp));
            }
        }
    };

    auto sortedNodes = graph.getSortedNodeIds();
    for (NodeId node : sortedNodes) {
        if (dfn[node] == -1) {
            tarjanDfs(tarjanDfs, node);
        }
    }

    std::sort(sccList.begin(), sccList.end());
    return sccList;
}

} // namespace DepResolver

