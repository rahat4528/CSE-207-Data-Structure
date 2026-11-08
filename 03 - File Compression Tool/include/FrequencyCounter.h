#ifndef HUFFMAN_FREQUENCY_COUNTER_H
#define HUFFMAN_FREQUENCY_COUNTER_H

#include "Common.h"
#include "IStream.h"
#include <array>
#include <cmath>

namespace HuffmanEngine {

// Custom Hash Map Node for symbol frequency lookup (DG1 Option B custom data structure)
struct FrequencyNode {
    byte_t symbol{0};
    frequency_t count{0};
    FrequencyNode* next{nullptr};
};

class CustomFrequencyMap {
private:
    static constexpr size_t BUCKET_COUNT = 64;
    std::array<FrequencyNode*, BUCKET_COUNT> buckets{};
    size_t uniqueCount{0};

    size_t hash(byte_t key) const {
        return static_cast<size_t>(key) % BUCKET_COUNT;
    }

public:
    CustomFrequencyMap();
    ~CustomFrequencyMap();
    CustomFrequencyMap(const CustomFrequencyMap&) = delete;
    CustomFrequencyMap& operator=(const CustomFrequencyMap&) = delete;

    void insertOrAdd(byte_t symbol, frequency_t amount = 1);
    frequency_t get(byte_t symbol) const;
    bool contains(byte_t symbol) const;
    size_t size() const { return uniqueCount; }
    void clear();
};

class FrequencyCounter {
private:
    std::array<frequency_t, ALPHABET_SIZE> frequencies{};
    filesize_t totalBytes{0};
    size_t distinctSymbols{0};
    CustomFrequencyMap customMap;

public:
    FrequencyCounter();

    void reset();
    void countByte(byte_t b, frequency_t count = 1);
    void countBuffer(const uint8_t* buffer, size_t size);
    Result<void> countStream(IInputStream& stream, size_t bufferSize = DEFAULT_BUFFER_SIZE);

    frequency_t getFrequency(byte_t symbol) const { return frequencies[symbol]; }
    const std::array<frequency_t, ALPHABET_SIZE>& getTable() const { return frequencies; }
    filesize_t getTotalBytes() const { return totalBytes; }
    size_t getDistinctSymbolCount() const { return distinctSymbols; }

    // Computes Shannon Entropy H(X) in bits per symbol (0.0 to 8.0)
    double calculateEntropy() const;

    // Huffman theoretical minimum bits needed
    uint64_t calculateTheoreticalMinimumBits() const;

    // Identifies symbol with highest and lowest non-zero frequency
    std::optional<std::pair<byte_t, frequency_t>> getMostFrequentSymbol() const;
    std::optional<std::pair<byte_t, frequency_t>> getLeastFrequentSymbol() const;

    const CustomFrequencyMap& getCustomMap() const { return customMap; }
};

} // namespace HuffmanEngine

#endif // HUFFMAN_FREQUENCY_COUNTER_H
