#include "TestRunner.h"
#include "FrequencyCounter.h"

using namespace HuffmanEngine;

TEST_CASE(test_frequency_empty_stream) {
    FrequencyCounter counter;
    MemoryInputStream stream("");
    auto res = counter.countStream(stream);
    ASSERT_TRUE(res.isOk);
    ASSERT_EQ(counter.getTotalBytes(), 0);
    ASSERT_EQ(counter.getDistinctSymbolCount(), 0);
    ASSERT_EQ(counter.calculateEntropy(), 0.0);
    ASSERT_EQ(counter.calculateTheoreticalMinimumBits(), 0);
    ASSERT_FALSE(counter.getMostFrequentSymbol().has_value());
}

TEST_CASE(test_frequency_single_symbol) {
    FrequencyCounter counter;
    std::string text(1000, 'Z');
    MemoryInputStream stream(text);
    auto res = counter.countStream(stream);
    ASSERT_TRUE(res.isOk);
    ASSERT_EQ(counter.getTotalBytes(), 1000);
    ASSERT_EQ(counter.getDistinctSymbolCount(), 1);
    ASSERT_EQ(counter.getFrequency('Z'), 1000);
    ASSERT_EQ(counter.calculateEntropy(), 0.0);
    ASSERT_TRUE(counter.getMostFrequentSymbol().has_value());
    ASSERT_EQ(counter.getMostFrequentSymbol()->first, 'Z');
    ASSERT_EQ(counter.getMostFrequentSymbol()->second, 1000);
}

TEST_CASE(test_frequency_two_symbols_entropy) {
    FrequencyCounter counter;
    // 500 'A's and 500 'B's -> Shannon entropy should be exactly 1.0 bit
    std::string text;
    for (int i = 0; i < 500; ++i) {
        text.push_back('A');
        text.push_back('B');
    }
    MemoryInputStream stream(text);
    counter.countStream(stream);
    ASSERT_EQ(counter.getTotalBytes(), 1000);
    ASSERT_EQ(counter.getDistinctSymbolCount(), 2);
    ASSERT_EQ(counter.getFrequency('A'), 500);
    ASSERT_EQ(counter.getFrequency('B'), 500);
    double entropy = counter.calculateEntropy();
    // Check if within 0.0001 of 1.0
    ASSERT_TRUE(std::abs(entropy - 1.0) < 0.0001);
    ASSERT_EQ(counter.calculateTheoreticalMinimumBits(), 1000);
}

TEST_CASE(test_frequency_all_256_bytes_uniform) {
    FrequencyCounter counter;
    std::vector<uint8_t> allBytes;
    for (int i = 0; i < 256; ++i) {
        allBytes.push_back(static_cast<uint8_t>(i));
    }
    MemoryInputStream stream(allBytes);
    counter.countStream(stream);
    ASSERT_EQ(counter.getTotalBytes(), 256);
    ASSERT_EQ(counter.getDistinctSymbolCount(), 256);
    double entropy = counter.calculateEntropy();
    // For uniform 256 symbols, entropy = log2(256) = 8.0 bits
    ASSERT_TRUE(std::abs(entropy - 8.0) < 0.0001);
    ASSERT_EQ(counter.calculateTheoreticalMinimumBits(), 256 * 8);

    // Verify custom map mirrors table
    ASSERT_EQ(counter.getCustomMap().size(), 256);
    for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(counter.getCustomMap().get(static_cast<byte_t>(i)), 1);
    }
}
