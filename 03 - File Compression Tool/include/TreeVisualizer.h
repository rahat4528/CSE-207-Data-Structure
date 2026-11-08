#ifndef HUFFMAN_TREE_VISUALIZER_H
#define HUFFMAN_TREE_VISUALIZER_H

#include "Common.h"
#include "HuffmanTree.h"
#include <string>
#include <iostream>

namespace HuffmanEngine {

class TreeVisualizer {
public:
    // Prints an ASCII / Unicode box-drawing tree representation to the console
    static void printAsciiTree(
        const HuffmanTree& tree,
        std::ostream& os = std::cout,
        size_t maxDepth = 6);

    // Generates a standalone, styled vector SVG diagram of the Huffman tree
    static Result<void> exportSvg(
        const HuffmanTree& tree,
        const std::string& outputPath,
        int maxDepth = 7);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_TREE_VISUALIZER_H
