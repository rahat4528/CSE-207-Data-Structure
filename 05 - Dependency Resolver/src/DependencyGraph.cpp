#include "DependencyGraph.h"
#include <queue>
#include <algorithm>

namespace DepResolver {

NodeId DependencyGraph::getOrAddNode(const std::string& packageName) {
    auto it = nameToId_.find(packageName);
    if (it != nameToId_.end()) {
        return it->second;
    }

    NodeId newId = static_cast<NodeId>(idToName_.size());
    nameToId_[packageName] = newId;
    idToName_.push_back(packageName);
    forwardAdj_.emplace_back();
    reverseAdj_.emplace_back();
    return newId;
}

std::optional<NodeId> DependencyGraph::findNodeId(const std::string& packageName) const {
    auto it = nameToId_.find(packageName);
    if (it != nameToId_.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::string& DependencyGraph::getNodeName(NodeId id) const {
    static const std::string empty;
    if (id < idToName_.size()) {
        return idToName_[id];
    }
    return empty;
}

size_t DependencyGraph::edgeCount() const {
    size_t count = 0;
    for (const auto& edges : forwardAdj_) {
        count += edges.size();
    }
    return count;
}

Result<void> DependencyGraph::addEdge(const std::string& fromPkg, const std::string& toPkg, 
                                     Constraint constraint, DependencyKind kind) {
    NodeId u = getOrAddNode(fromPkg);
    NodeId v = getOrAddNode(toPkg);

    // Check if edge already exists in forwardAdj_[u]
    for (auto& edge : forwardAdj_[u]) {
        if (edge.target == v) {
            // Duplicate edge: merge constraints via intersection
            auto inter = edge.constraint.intersect(constraint);
            if (!inter.has_value()) {
                return Result<void>::err(
                    ErrorCode::VERSION_CONFLICT,
                    "Conflicting duplicate dependency constraint between " + fromPkg + " and " + toPkg
                );
            }
            edge.constraint = inter.value();
            // Update corresponding reverse edge as well
            for (auto& revEdge : reverseAdj_[v]) {
                if (revEdge.target == u) {
                    revEdge.constraint = inter.value();
                    break;
                }
            }
            return Result<void>::ok();
        }
    }

    // Add new forward edge u -> v
    forwardAdj_[u].emplace_back(v, constraint, kind);
    // Add new reverse edge v <- u
    reverseAdj_[v].emplace_back(u, constraint, kind);

    return Result<void>::ok();
}

Result<void> DependencyGraph::removeEdge(const std::string& fromPkg, const std::string& toPkg) {
    auto uOpt = findNodeId(fromPkg);
    auto vOpt = findNodeId(toPkg);
    if (!uOpt.has_value() || !vOpt.has_value()) {
        return Result<void>::err(ErrorCode::DEPENDENCY_NOT_FOUND, "Edge does not exist between " + fromPkg + " and " + toPkg);
    }

    NodeId u = uOpt.value();
    NodeId v = vOpt.value();

    // Remove from forwardAdj_[u]
    auto fIt = std::find_if(forwardAdj_[u].begin(), forwardAdj_[u].end(), [v](const GraphEdge& e) {
        return e.target == v;
    });
    if (fIt == forwardAdj_[u].end()) {
        return Result<void>::err(ErrorCode::DEPENDENCY_NOT_FOUND, "Forward edge not found");
    }
    forwardAdj_[u].erase(fIt);

    // Remove from reverseAdj_[v]
    auto rIt = std::find_if(reverseAdj_[v].begin(), reverseAdj_[v].end(), [u](const GraphEdge& e) {
        return e.target == u;
    });
    if (rIt != reverseAdj_[v].end()) {
        reverseAdj_[v].erase(rIt);
    }

    return Result<void>::ok();
}

bool DependencyGraph::hasEdge(NodeId from, NodeId to) const {
    if (from >= forwardAdj_.size()) return false;
    for (const auto& e : forwardAdj_[from]) {
        if (e.target == to) return true;
    }
    return false;
}

bool DependencyGraph::hasSelfDependency(NodeId id) const {
    return hasEdge(id, id);
}

const std::vector<GraphEdge>& DependencyGraph::getDependencies(NodeId u) const {
    static const std::vector<GraphEdge> empty;
    if (u < forwardAdj_.size()) {
        return forwardAdj_[u];
    }
    return empty;
}

const std::vector<GraphEdge>& DependencyGraph::getDependents(NodeId v) const {
    static const std::vector<GraphEdge> empty;
    if (v < reverseAdj_.size()) {
        return reverseAdj_[v];
    }
    return empty;
}

std::vector<NodeId> DependencyGraph::getSortedNodeIds() const {
    std::vector<NodeId> ids(idToName_.size());
    for (size_t i = 0; i < ids.size(); ++i) {
        ids[i] = static_cast<NodeId>(i);
    }

    // Sort by name for strict determinism
    std::sort(ids.begin(), ids.end(), [this](NodeId a, NodeId b) {
        return idToName_[a] < idToName_[b];
    });

    return ids;
}

std::vector<std::string> DependencyGraph::getAllDependenciesTransitive(const std::string& pkg) const {
    auto startOpt = findNodeId(pkg);
    if (!startOpt.has_value()) return {};

    std::vector<std::string> result;
    std::vector<bool> visited(idToName_.size(), false);
    std::queue<NodeId> q;

    visited[startOpt.value()] = true;
    q.push(startOpt.value());

    while (!q.empty()) {
        NodeId curr = q.front();
        q.pop();

        for (const auto& edge : forwardAdj_[curr]) {
            if (!visited[edge.target]) {
                visited[edge.target] = true;
                result.push_back(idToName_[edge.target]);
                q.push(edge.target);
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> DependencyGraph::getAllDependentsTransitive(const std::string& pkg) const {
    auto startOpt = findNodeId(pkg);
    if (!startOpt.has_value()) return {};

    std::vector<std::string> result;
    std::vector<bool> visited(idToName_.size(), false);
    std::queue<NodeId> q;

    visited[startOpt.value()] = true;
    q.push(startOpt.value());

    while (!q.empty()) {
        NodeId curr = q.front();
        q.pop();

        for (const auto& edge : reverseAdj_[curr]) {
            if (!visited[edge.target]) {
                visited[edge.target] = true;
                result.push_back(idToName_[edge.target]);
                q.push(edge.target);
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

void DependencyGraph::clear() {
    nameToId_.clear();
    idToName_.clear();
    forwardAdj_.clear();
    reverseAdj_.clear();
}

} // namespace DepResolver

