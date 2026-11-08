#pragma once

#include "Common.h"
#include "DependencyGraph.h"
#include "TopologicalSorter.h"
#include <string>
#include <unordered_set>

namespace DepResolver {

class Visualizer {
public:
    // Terminal Unicode/ASCII Tree Display (Marks shared nodes and cycles)
    static std::string renderAsciiTree(const DependencyGraph& graph, const std::string& rootPkg, size_t maxDepth = 15);

    // Reverse Tree: "Why is package X needed?" (Walks reverse dependencies to root)
    static std::string renderWhyTree(const DependencyGraph& graph, const std::string& targetPkg);

    // Standalone Self-Generated SVG Graph Visualizer (E7 / TG7)
    // Emits valid vector graphics viewable directly in browser without external tools
    static std::string renderSvg(const DependencyGraph& graph, const std::string& title = "Dependency Graph");
};

} // namespace DepResolver

