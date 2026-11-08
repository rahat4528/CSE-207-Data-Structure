#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#include "Common.h"
#include "HuffmanNode.h"
#include "PriorityQueue.h"
#include <array>
#include <string>
#include <vector>

namespace HuffmanEngine {

struct SymbolCodeInfo {
    byte_t symbol{0};
    frequency_t frequency{0};
    BitCode code;
    uint16_t length{0};
};

class HuffmanTree {
private:
    HuffmanNode* root{nullptr};
    std::array<BitCode, ALPHABET_SIZE> codeTable{};
    std::array<bool, ALPHABET_SIZE> activeSymbols{};
    size_t distinctCount{0};
    uint32_t creationCounter{0};

    void cleanup();

public:
    HuffmanTree();
    ~HuffmanTree();

    HuffmanTree(const HuffmanTree&) = delete;
    HuffmanTree& operator=(const HuffmanTree&) = delete;

    // Builds the tree from symbol frequency counts
    Result<void> buildFromFrequencies(const std::array<frequency_t, ALPHABET_SIZE>& frequencies);

    // Generates prefix-free binary codes iteratively
    void generateCodes();

    // Verification and metric analysis
    bool validatePrefixProperty() const;
    bool validateKraftInequality() const;
    uint64_t calculateWeightedPathLength(const std::array<frequency_t, ALPHABET_SIZE>& frequencies) const;
    double calculateAverageCodeLength(const std::array<frequency_t, ALPHABET_SIZE>& frequencies, filesize_t totalBytes) const;

    const BitCode& getCode(byte_t symbol) const { return codeTable[symbol]; }
    const std::array<BitCode, ALPHABET_SIZE>& getCodeTable() const { return codeTable; }
    bool hasSymbol(byte_t symbol) const { return activeSymbols[symbol]; }
    size_t getDistinctCount() const { return distinctCount; }
    const HuffmanNode* getRoot() const { return root; }
    bool isEmpty() const { return root == nullptr; }

    // Collects all active symbol code records sorted by frequency (descending)
    std::vector<SymbolCodeInfo> getSortedCodeInfo(const std::array<frequency_t, ALPHABET_SIZE>& frequencies) const;
};

} // namespace HuffmanEngine

#endif // HUFFMAN_TREE_H
