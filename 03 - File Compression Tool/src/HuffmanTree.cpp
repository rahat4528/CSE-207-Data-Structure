#include "HuffmanTree.h"
#include <stack>
#include <cmath>
#include <algorithm>

namespace HuffmanEngine {

HuffmanTree::HuffmanTree() {
    codeTable.fill(BitCode{});
    activeSymbols.fill(false);
}

HuffmanTree::~HuffmanTree() {
    cleanup();
}

void HuffmanTree::cleanup() {
    delete root;
    root = nullptr;
    codeTable.fill(BitCode{});
    activeSymbols.fill(false);
    distinctCount = 0;
    creationCounter = 0;
}

Result<void> HuffmanTree::buildFromFrequencies(const std::array<frequency_t, ALPHABET_SIZE>& frequencies) {
    cleanup();

    HuffmanPriorityQueue pq;

    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (frequencies[i] > 0) {
            byte_t sym = static_cast<byte_t>(i);
            HuffmanNode* leaf = new HuffmanNode(sym, frequencies[i], ++creationCounter);
            pq.push(leaf);
            activeSymbols[i] = true;
            distinctCount++;
        }
    }

    if (pq.empty()) {
        root = nullptr;
        return Result<void>::ok();
    }

    if (pq.size() == 1) {
        // Single symbol special convention: root is the leaf node
        root = pq.pop();
        generateCodes();
        return Result<void>::ok();
    }

    while (pq.size() > 1) {
        HuffmanNode* left = pq.pop();
        HuffmanNode* right = pq.pop();
        HuffmanNode* parent = new HuffmanNode(left, right, ++creationCounter);
        pq.push(parent);
    }

    root = pq.pop();
    generateCodes();
    return Result<void>::ok();
}

void HuffmanTree::generateCodes() {
    if (!root) return;

    if (root->isLeaf) {
        // Single-symbol convention: assign 1-bit code '0'
        BitCode singleCode;
        singleCode.appendBit(false);
        codeTable[root->symbol] = singleCode;
        return;
    }

    // Iterative tree traversal using an explicit stack
    struct Frame {
        const HuffmanNode* node;
        BitCode code;
    };

    std::stack<Frame> st;
    st.push({root, BitCode{}});

    while (!st.empty()) {
        Frame curr = st.top();
        st.pop();

        if (curr.node->isLeaf) {
            codeTable[curr.node->symbol] = curr.code;
            continue;
        }

        // Push right child first (with bit 1) so left child is processed first
        if (curr.node->right) {
            BitCode rCode = curr.code;
            rCode.appendBit(true); // Right branch = 1
            st.push({curr.node->right, rCode});
        }

        if (curr.node->left) {
            BitCode lCode = curr.code;
            lCode.appendBit(false); // Left branch = 0
            st.push({curr.node->left, lCode});
        }
    }
}

bool HuffmanTree::validatePrefixProperty() const {
    if (distinctCount <= 1) return true;

    std::vector<byte_t> syms;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (activeSymbols[i]) syms.push_back(static_cast<byte_t>(i));
    }

    for (size_t i = 0; i < syms.size(); ++i) {
        const BitCode& c1 = codeTable[syms[i]];
        for (size_t j = i + 1; j < syms.size(); ++j) {
            const BitCode& c2 = codeTable[syms[j]];
            // If lengths are equal and bits are identical -> conflict
            if (c1.length == c2.length) {
                if (c1 == c2) return false;
                continue;
            }
            // Check if shorter is prefix of longer
            size_t minLen = std::min(c1.length, c2.length);
            bool isPrefix = true;
            for (size_t b = 0; b < minLen; ++b) {
                if (c1.getBit(b) != c2.getBit(b)) {
                    isPrefix = false;
                    break;
                }
            }
            if (isPrefix) return false; // Violation: one code is prefix of another
        }
    }
    return true;
}

bool HuffmanTree::validateKraftInequality() const {
    if (distinctCount == 0) return true;
    if (distinctCount == 1) return true; // 2^(-1) = 0.5 <= 1.0

    double sum = 0.0;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (activeSymbols[i]) {
            uint16_t len = codeTable[i].length;
            sum += std::pow(2.0, -static_cast<double>(len));
        }
    }
    return sum <= 1.00000001; // Kraft inequality <= 1
}

uint64_t HuffmanTree::calculateWeightedPathLength(const std::array<frequency_t, ALPHABET_SIZE>& frequencies) const {
    uint64_t wpl = 0;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (frequencies[i] > 0) {
            wpl += frequencies[i] * codeTable[i].length;
        }
    }
    return wpl;
}

double HuffmanTree::calculateAverageCodeLength(const std::array<frequency_t, ALPHABET_SIZE>& frequencies, filesize_t totalBytes) const {
    if (totalBytes == 0) return 0.0;
    uint64_t wpl = calculateWeightedPathLength(frequencies);
    return static_cast<double>(wpl) / static_cast<double>(totalBytes);
}

std::vector<SymbolCodeInfo> HuffmanTree::getSortedCodeInfo(const std::array<frequency_t, ALPHABET_SIZE>& frequencies) const {
    std::vector<SymbolCodeInfo> result;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (activeSymbols[i]) {
            SymbolCodeInfo info;
            info.symbol = static_cast<byte_t>(i);
            info.frequency = frequencies[i];
            info.code = codeTable[i];
            info.length = codeTable[i].length;
            result.push_back(info);
        }
    }
    std::sort(result.begin(), result.end(), [](const SymbolCodeInfo& a, const SymbolCodeInfo& b) {
        if (a.frequency != b.frequency) return a.frequency > b.frequency; // Descending frequency
        return a.symbol < b.symbol;
    });
    return result;
}

} // namespace HuffmanEngine
