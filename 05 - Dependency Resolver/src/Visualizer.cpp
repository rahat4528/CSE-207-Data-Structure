#include "Visualizer.h"
#include <sstream>
#include <algorithm>
#include <map>

namespace DepResolver {

namespace {

void buildAsciiTreeRecursive(
    const DependencyGraph& graph,
    NodeId u,
    std::string prefix,
    bool isLast,
    std::unordered_set<NodeId>& visitedGlobal,
    std::unordered_set<NodeId>& activePath,
    size_t depth,
    size_t maxDepth,
    std::ostringstream& ss) 
{
    if (depth > maxDepth) {
        ss << prefix << (isLast ? "\\-- " : "|-- ") << "... (max depth reached)\n";
        return;
    }

    std::string pkgName = graph.getNodeName(u);
    ss << prefix << (isLast ? "\\-- " : "|-- ") << pkgName;

    // Check for circular dependency
    if (activePath.find(u) != activePath.end()) {
        ss << " [CYCLE DETECTED]\n";
        return;
    }

    // Check for already visited diamond/shared dependency
    if (visitedGlobal.find(u) != visitedGlobal.end() && depth > 0) {
        ss << " [shared/deduped]\n";
        return;
    }

    visitedGlobal.insert(u);
    activePath.insert(u);
    ss << "\n";

    const auto& edges = graph.getDependencies(u);
    std::vector<NodeId> targets;
    for (const auto& e : edges) targets.push_back(e.target);
    std::sort(targets.begin(), targets.end(), [&](NodeId a, NodeId b) {
        return graph.getNodeName(a) < graph.getNodeName(b);
    });

    for (size_t i = 0; i < targets.size(); ++i) {
        bool lastChild = (i + 1 == targets.size());
        std::string nextPrefix = prefix + (isLast ? "    " : "|   ");
        buildAsciiTreeRecursive(graph, targets[i], nextPrefix, lastChild, visitedGlobal, activePath, depth + 1, maxDepth, ss);
    }

    activePath.erase(u);
}

} // namespace

std::string Visualizer::renderAsciiTree(const DependencyGraph& graph, const std::string& rootPkg, size_t maxDepth) {
    auto rootOpt = graph.findNodeId(rootPkg);
    if (!rootOpt.has_value()) {
        return "Package '" + rootPkg + "' not found in graph.\n";
    }

    std::ostringstream ss;
    ss << rootPkg << "\n";

    std::unordered_set<NodeId> visitedGlobal;
    std::unordered_set<NodeId> activePath;
    visitedGlobal.insert(rootOpt.value());
    activePath.insert(rootOpt.value());

    const auto& edges = graph.getDependencies(rootOpt.value());
    std::vector<NodeId> targets;
    for (const auto& e : edges) targets.push_back(e.target);
    std::sort(targets.begin(), targets.end(), [&](NodeId a, NodeId b) {
        return graph.getNodeName(a) < graph.getNodeName(b);
    });

    for (size_t i = 0; i < targets.size(); ++i) {
        bool lastChild = (i + 1 == targets.size());
        buildAsciiTreeRecursive(graph, targets[i], "", lastChild, visitedGlobal, activePath, 1, maxDepth, ss);
    }

    return ss.str();
}

std::string Visualizer::renderWhyTree(const DependencyGraph& graph, const std::string& targetPkg) {
    auto targetOpt = graph.findNodeId(targetPkg);
    if (!targetOpt.has_value()) {
        return "Package '" + targetPkg + "' not found in graph.\n";
    }

    std::ostringstream ss;
    ss << "=== Reverse Dependency Trace for '" << targetPkg << "' ===\n";

    auto dependents = graph.getAllDependentsTransitive(targetPkg);
    if (dependents.empty()) {
        ss << "  (Directly requested package: no other package depends on it)\n";
        return ss.str();
    }

    ss << "Package '" << targetPkg << "' is required by:\n";
    for (const auto& dep : dependents) {
        ss << "  <-- " << dep << "\n";
    }

    return ss.str();
}

std::string Visualizer::renderSvg(const DependencyGraph& graph, const std::string& title) {
    std::ostringstream ss;
    size_t n = graph.nodeCount();
    if (n == 0) {
        ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" height=\"100\">"
           << "<text x=\"20\" y=\"50\" font-family=\"sans-serif\">Empty Graph</text></svg>";
        return ss.str();
    }

    // Assign layout coordinates based on topological layers
    auto layersRes = TopologicalSorter::getParallelLayers(graph);
    std::vector<std::vector<std::string>> layers;
    if (layersRes.isOk()) {
        layers = layersRes.value();
    } else {
        // Fallback for cyclic graphs: put in single column
        layers.push_back({});
        for (NodeId id : graph.getSortedNodeIds()) {
            layers.back().push_back(graph.getNodeName(id));
        }
    }

    size_t maxLayerWidth = 1;
    for (const auto& layer : layers) {
        maxLayerWidth = std::max(maxLayerWidth, layer.size());
    }

    int nodeW = 140;
    int nodeH = 45;
    int gapX = 100;
    int gapY = 50;

    int totalW = static_cast<int>(layers.size()) * (nodeW + gapX) + 80;
    int totalH = static_cast<int>(maxLayerWidth) * (nodeH + gapY) + 100;

    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << totalW 
       << "\" height=\"" << totalH << "\" viewBox=\"0 0 " << totalW << " " << totalH << "\">\n";

    // Styles & Marker for arrowheads
    ss << "<defs>\n";
    ss << "  <marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"10\" refY=\"5\" markerWidth=\"6\" markerHeight=\"6\" orient=\"auto-start-reverse\">\n";
    ss << "    <path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"#4A5568\" />\n";
    ss << "  </marker>\n";
    ss << "</defs>\n";

    ss << "<rect width=\"100%\" height=\"100%\" fill=\"#F7FAFC\"/>\n";
    ss << "<text x=\"30\" y=\"40\" font-family=\"Arial, sans-serif\" font-size=\"20\" font-weight=\"bold\" fill=\"#2D3748\">" 
       << title << "</text>\n";

    // Map package name to (x, y) coordinates
    std::map<std::string, std::pair<int, int>> coords;
    for (size_t col = 0; col < layers.size(); ++col) {
        int x = 40 + static_cast<int>(col) * (nodeW + gapX);
        for (size_t row = 0; row < layers[col].size(); ++row) {
            int y = 70 + static_cast<int>(row) * (nodeH + gapY);
            coords[layers[col][row]] = {x, y};
        }
    }

    // Draw dependency edges (arrows from u to v)
    for (size_t u = 0; u < n; ++u) {
        std::string uName = graph.getNodeName(static_cast<NodeId>(u));
        auto uCoord = coords[uName];

        for (const auto& edge : graph.getDependencies(static_cast<NodeId>(u))) {
            std::string vName = graph.getNodeName(edge.target);
            auto vCoord = coords[vName];

            int x1 = uCoord.first + nodeW;
            int y1 = uCoord.second + nodeH / 2;
            int x2 = vCoord.first;
            int y2 = vCoord.second + nodeH / 2;

            ss << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 
               << "\" stroke=\"#718096\" stroke-width=\"2\" marker-end=\"url(#arrow)\"/>\n";
        }
    }

    // Draw node boxes
    for (const auto& [name, pt] : coords) {
        ss << "<g transform=\"translate(" << pt.first << "," << pt.second << ")\">\n";
        ss << "  <rect width=\"" << nodeW << "\" height=\"" << nodeH 
           << "\" rx=\"8\" ry=\"8\" fill=\"#EDF2F7\" stroke=\"#3182CE\" stroke-width=\"2\"/>\n";
        ss << "  <text x=\"" << nodeW / 2 << "\" y=\"" << nodeH / 2 + 5 
           << "\" font-family=\"Arial, sans-serif\" font-size=\"13\" font-weight=\"bold\" fill=\"#2B6CB0\" text-anchor=\"middle\">" 
           << name << "</text>\n";
        ss << "</g>\n";
    }

    ss << "</svg>\n";
    return ss.str();
}

} // namespace DepResolver

