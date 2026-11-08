#include "TreeVisualizer.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>

namespace HuffmanEngine {

namespace {

void printNodeAscii(
    const HuffmanNode* node,
    std::ostream& os,
    const std::string& prefix,
    bool isLeft,
    bool hasParent,
    size_t depth,
    size_t maxDepth) {

    if (!node) return;

    os << prefix;
    if (hasParent) {
        os << (isLeft ? "|-- [0] " : "`-- [1] ");
    }

    if (node->isLeaf) {
        char ch = static_cast<char>(node->symbol);
        std::string symStr;
        if (ch >= 32 && ch <= 126 && ch != '\\' && ch != '\'') {
            symStr = std::string("'") + ch + "'";
        } else {
            std::ostringstream ss;
            ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(node->symbol);
            symStr = ss.str();
        }
        os << "LEAF: " << symStr << " (wt: " << node->weight << ")\n";
        return;
    }

    os << "NODE (wt: " << node->weight << ")\n";

    if (depth >= maxDepth) {
        os << prefix << (isLeft ? "|   " : "    ") << "[... pruned at depth " << maxDepth << " ...]\n";
        return;
    }

    std::string childPrefix = prefix + (hasParent ? (isLeft ? "|   " : "    ") : "");
    if (node->left) {
        printNodeAscii(node->left, os, childPrefix, true, true, depth + 1, maxDepth);
    }
    if (node->right) {
        printNodeAscii(node->right, os, childPrefix, false, true, depth + 1, maxDepth);
    }
}

struct SvgNode {
    double x;
    double y;
    const HuffmanNode* node;
    std::string edgeLabel;
    int parentIndex{-1};
};

void layoutSvg(
    const HuffmanNode* node,
    int depth,
    int maxDepth,
    double leftX,
    double rightX,
    double y,
    int parentIdx,
    const std::string& edgeLabel,
    std::vector<SvgNode>& nodes) {

    if (!node || depth > maxDepth) return;

    double midX = (leftX + rightX) / 2.0;
    int currIdx = static_cast<int>(nodes.size());
    nodes.push_back({midX, y, node, edgeLabel, parentIdx});

    if (node->isLeaf || depth == maxDepth) return;

    double nextY = y + 70.0;
    if (node->left) {
        layoutSvg(node->left, depth + 1, maxDepth, leftX, midX, nextY, currIdx, "0", nodes);
    }
    if (node->right) {
        layoutSvg(node->right, depth + 1, maxDepth, midX, rightX, nextY, currIdx, "1", nodes);
    }
}

} // anonymous namespace

void TreeVisualizer::printAsciiTree(const HuffmanTree& tree, std::ostream& os, size_t maxDepth) {
    const HuffmanNode* root = tree.getRoot();
    if (!root) {
        os << "[Empty Huffman Tree]\n";
        return;
    }
    os << "=== HUFFMAN TREE VISUALIZATION ===\n";
    printNodeAscii(root, os, "", false, false, 0, maxDepth);
    os << "==================================\n";
}

Result<void> TreeVisualizer::exportSvg(
    const HuffmanTree& tree,
    const std::string& outputPath,
    int maxDepth) {

    const HuffmanNode* root = tree.getRoot();
    if (!root) {
        return Result<void>::fail(ErrorCode::ERR_INVALID_PARAMETER, "Cannot export SVG for an empty tree");
    }

    std::vector<SvgNode> nodes;
    layoutSvg(root, 0, maxDepth, 20.0, 1180.0, 50.0, -1, "", nodes);

    double maxY = 100.0;
    for (const auto& n : nodes) {
        if (n.y > maxY) maxY = n.y;
    }
    double svgWidth = 1200.0;
    double svgHeight = maxY + 80.0;

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        return Result<void>::fail(ErrorCode::ERR_FILE_WRITE_FAILED, "Failed to open SVG output file: " + outputPath);
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    out << "<svg width=\"" << svgWidth << "\" height=\"" << svgHeight << "\" viewBox=\"0 0 " << svgWidth << " " << svgHeight
        << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    out << "<style>\n"
        << "  .edge { stroke: #64748b; stroke-width: 2; }\n"
        << "  .edge-label { font-family: monospace; font-size: 12px; fill: #ef4444; font-weight: bold; }\n"
        << "  .node-circle { fill: #3b82f6; stroke: #1e3a8a; stroke-width: 2; }\n"
        << "  .leaf-circle { fill: #10b981; stroke: #065f46; stroke-width: 2; }\n"
        << "  .node-text { font-family: sans-serif; font-size: 11px; fill: #ffffff; text-anchor: middle; dominant-baseline: central; }\n"
        << "  .leaf-label { font-family: monospace; font-size: 12px; fill: #0f172a; text-anchor: middle; font-weight: bold; }\n"
        << "</style>\n";
    out << "<rect width=\"100%\" height=\"100%\" fill=\"#f8fafc\"/>\n";

    // 1. Draw Edges
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].parentIndex >= 0) {
            const auto& p = nodes[nodes[i].parentIndex];
            const auto& c = nodes[i];
            out << "<line x1=\"" << p.x << "\" y1=\"" << p.y << "\" x2=\"" << c.x << "\" y2=\"" << c.y << "\" class=\"edge\"/>\n";
            double lx = (p.x + c.x) / 2.0 + (c.x > p.x ? 8.0 : -14.0);
            double ly = (p.y + c.y) / 2.0 - 4.0;
            out << "<text x=\"" << lx << "\" y=\"" << ly << "\" class=\"edge-label\">" << c.edgeLabel << "</text>\n";
        }
    }

    // 2. Draw Nodes
    for (const auto& n : nodes) {
        if (n.node->isLeaf) {
            out << "<circle cx=\"" << n.x << "\" cy=\"" << n.y << "\" r=\"18\" class=\"leaf-circle\"/>\n";
            char ch = static_cast<char>(n.node->symbol);
            std::string disp;
            if (ch >= 32 && ch <= 126 && ch != '<' && ch != '>' && ch != '&') {
                disp = std::string(1, ch);
            } else {
                std::ostringstream ss;
                ss << std::hex << std::uppercase << static_cast<int>(n.node->symbol);
                disp = ss.str();
            }
            out << "<text x=\"" << n.x << "\" y=\"" << n.y << "\" class=\"node-text\">" << disp << "</text>\n";
            out << "<text x=\"" << n.x << "\" y=\"" << n.y + 28.0 << "\" class=\"leaf-label\">" << n.node->weight << "</text>\n";
        } else {
            out << "<circle cx=\"" << n.x << "\" cy=\"" << n.y << "\" r=\"16\" class=\"node-circle\"/>\n";
            out << "<text x=\"" << n.x << "\" y=\"" << n.y << "\" class=\"node-text\">" << n.node->weight << "</text>\n";
        }
    }

    out << "</svg>\n";
    return Result<void>::ok();
}

} // namespace HuffmanEngine
