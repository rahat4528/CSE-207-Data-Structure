#pragma once

#include "Common.h"
#include "Constraint.h"
#include "Package.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <optional>
#include <cstdint>

namespace DepResolver {

using NodeId = uint32_t;
constexpr NodeId INVALID_NODE_ID = UINT32_MAX;

// ============================================================================
// Directed Graph Edge with Metadata
// ============================================================================
struct GraphEdge {
    NodeId target{INVALID_NODE_ID};
    Constraint constraint;
    DependencyKind kind{DependencyKind::RUNTIME_DEP};

    GraphEdge() = default;
    GraphEdge(NodeId to, Constraint c, DependencyKind k = DependencyKind::RUNTIME_DEP)
        : target(to), constraint(std::move(c)), kind(k) {}
};

// ============================================================================
// Deterministic Dual-Adjacency Directed Dependency Graph
// Edge u -> v strictly means "u depends on v"
// ============================================================================
class DependencyGraph {
private:
    std::unordered_map<std::string, NodeId> nameToId_;
    std::vector<std::string> idToName_;

    // Forward adjacency: forwardAdj_[u] = list of packages u depends on
    std::vector<std::vector<GraphEdge>> forwardAdj_;

    // Reverse adjacency: reverseAdj_[v] = list of packages that depend on v ("who requires v?")
    std::vector<std::vector<GraphEdge>> reverseAdj_;

public:
    DependencyGraph() = default;

    // Node Interning
    NodeId getOrAddNode(const std::string& packageName);
    std::optional<NodeId> findNodeId(const std::string& packageName) const;
    const std::string& getNodeName(NodeId id) const;

    size_t nodeCount() const { return idToName_.size(); }
    size_t edgeCount() const;

    // Edge Operations: u -> v ("u depends on v")
    Result<void> addEdge(const std::string& fromPkg, const std::string& toPkg, 
                         Constraint constraint = Constraint::any(), 
                         DependencyKind kind = DependencyKind::RUNTIME_DEP);

    Result<void> removeEdge(const std::string& fromPkg, const std::string& toPkg);

    bool hasEdge(NodeId from, NodeId to) const;
    bool hasSelfDependency(NodeId id) const;

    // Adjacency Access
    const std::vector<GraphEdge>& getDependencies(NodeId u) const; // Forward edges
    const std::vector<GraphEdge>& getDependents(NodeId v) const;   // Reverse edges ("why is v needed?")

    // Sorted Node IDs for Total Determinism
    std::vector<NodeId> getSortedNodeIds() const;

    // Transitive Reachability Queries (O(V + E))
    std::vector<std::string> getAllDependenciesTransitive(const std::string& pkg) const;
    std::vector<std::string> getAllDependentsTransitive(const std::string& pkg) const;

    void clear();
};

} // namespace DepResolver

