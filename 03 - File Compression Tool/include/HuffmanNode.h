#ifndef HUFFMAN_NODE_H
#define HUFFMAN_NODE_H

#include "Common.h"
#include <memory>
#include <algorithm>

namespace HuffmanEngine {

struct HuffmanNode {
    frequency_t weight{0};
    byte_t symbol{0};
    bool isLeaf{false};
    uint16_t minSymbol{0}; // For deterministic tie-breaking
    uint32_t creationId{0};
    HuffmanNode* left{nullptr};
    HuffmanNode* right{nullptr};

    // Constructor for leaf
    HuffmanNode(byte_t sym, frequency_t wt, uint32_t id)
        : weight(wt), symbol(sym), isLeaf(true), minSymbol(sym), creationId(id), left(nullptr), right(nullptr) {}

    // Constructor for internal node
    HuffmanNode(HuffmanNode* l, HuffmanNode* r, uint32_t id)
        : weight(l->weight + r->weight),
          symbol(0),
          isLeaf(false),
          minSymbol(std::min(l->minSymbol, r->minSymbol)),
          creationId(id),
          left(l),
          right(r) {}

    ~HuffmanNode() {
        delete left;
        delete right;
    }

    // Disable copy to avoid double frees
    HuffmanNode(const HuffmanNode&) = delete;
    HuffmanNode& operator=(const HuffmanNode&) = delete;
};

// Strict total order comparator for Min Heap:
// 1. Lower weight comes first
// 2. Smaller minimum contained symbol comes first
// 3. Earlier creation ID comes first
struct HuffmanNodeComparator {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        if (a->weight != b->weight) {
            return a->weight < b->weight;
        }
        if (a->minSymbol != b->minSymbol) {
            return a->minSymbol < b->minSymbol;
        }
        return a->creationId < b->creationId;
    }
};

} // namespace HuffmanEngine

#endif // HUFFMAN_NODE_H
