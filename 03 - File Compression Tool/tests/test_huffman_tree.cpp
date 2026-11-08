#include "TestRunner.h"
#include "HuffmanTree.h"
#include "FrequencyCounter.h"

using namespace HuffmanEngine;

TEST_CASE(test_huffman_tree_empty_and_single_symbol) {
    HuffmanTree emptyTree;
    std::array<frequency_t, ALPHABET_SIZE> emptyFreq{};
    emptyFreq.fill(0);
    auto res = emptyTree.buildFromFrequencies(emptyFreq);
    ASSERT_TRUE(res.isOk);
    ASSERT_TRUE(emptyTree.isEmpty());
    ASSERT_EQ(emptyTree.getDistinctCount(), 0);

    // Single symbol test
    HuffmanTree singleTree;
    std::array<frequency_t, ALPHABET_SIZE> singleFreq{};
    singleFreq.fill(0);
    singleFreq['A'] = 500;
    res = singleTree.buildFromFrequencies(singleFreq);
    ASSERT_TRUE(res.isOk);
    ASSERT_FALSE(singleTree.isEmpty());
    ASSERT_EQ(singleTree.getDistinctCount(), 1);
    ASSERT_TRUE(singleTree.hasSymbol('A'));
    const BitCode& codeA = singleTree.getCode('A');
    ASSERT_EQ(codeA.length, 1);
    ASSERT_EQ(codeA.toString(), "0");
    ASSERT_TRUE(singleTree.validateKraftInequality());
    ASSERT_TRUE(singleTree.validatePrefixProperty());
}

TEST_CASE(test_huffman_tree_prefix_and_kraft_invariants) {
    HuffmanTree tree;
    FrequencyCounter counter;
    std::string sample = "ABRACADABRA_SIM_SALA_BIM_HUFFMAN_CODING_ENGINE_TEST_DATA";
    MemoryInputStream stream(sample);
    counter.countStream(stream);

    auto res = tree.buildFromFrequencies(counter.getTable());
    ASSERT_TRUE(res.isOk);
    ASSERT_TRUE(tree.validatePrefixProperty());
    ASSERT_TRUE(tree.validateKraftInequality());

    // Most frequent symbol should have shortest code
    auto mostFreq = counter.getMostFrequentSymbol();
    ASSERT_TRUE(mostFreq.has_value());
    uint16_t topLen = tree.getCode(mostFreq->first).length;

    auto leastFreq = counter.getLeastFrequentSymbol();
    ASSERT_TRUE(leastFreq.has_value());
    uint16_t bottomLen = tree.getCode(leastFreq->first).length;

    ASSERT_LE(topLen, bottomLen);
}

TEST_CASE(test_huffman_tree_determinism_and_skewed_distribution) {
    // Skewed Fibonacci frequencies: 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89
    std::array<frequency_t, ALPHABET_SIZE> freqs{};
    freqs.fill(0);
    std::vector<frequency_t> fib = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89};
    for (size_t i = 0; i < fib.size(); ++i) {
        freqs[static_cast<byte_t>('A' + i)] = fib[i];
    }

    HuffmanTree tree1;
    tree1.buildFromFrequencies(freqs);

    HuffmanTree tree2;
    tree2.buildFromFrequencies(freqs);

    ASSERT_TRUE(tree1.validatePrefixProperty());
    ASSERT_TRUE(tree1.validateKraftInequality());

    // Determinism test: both trees must produce identical codes
    for (size_t i = 0; i < fib.size(); ++i) {
        byte_t sym = static_cast<byte_t>('A' + i);
        ASSERT_EQ(tree1.getCode(sym).length, tree2.getCode(sym).length);
        ASSERT_EQ(tree1.getCode(sym).toString(), tree2.getCode(sym).toString());
    }

    // Skewed tree depth should be equal to fib.size() - 1 or fib.size()
    uint16_t maxCodeLen = 0;
    for (size_t i = 0; i < fib.size(); ++i) {
        byte_t sym = static_cast<byte_t>('A' + i);
        maxCodeLen = std::max(maxCodeLen, tree1.getCode(sym).length);
    }
    ASSERT_GE(maxCodeLen, 9); // Skewed tree creates deep branching
}
