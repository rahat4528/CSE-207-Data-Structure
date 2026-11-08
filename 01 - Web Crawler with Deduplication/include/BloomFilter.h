#ifndef CRAWLER_BLOOM_FILTER_H
#define CRAWLER_BLOOM_FILTER_H

#include "Common.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string_view>
#include <cmath>
#include <shared_mutex>
#include <mutex>

namespace crawler {

class BloomFilter {
public:
    BloomFilter(size_t expectedItems, double targetFpr);
    BloomFilter(size_t numBits, size_t numHashes);

    void insert(std::string_view key);
    bool possiblyContains(std::string_view key) const;

    size_t bitCount() const noexcept { return m_numBits; }
    size_t hashCount() const noexcept { return m_numHashes; }
    size_t itemCount() const noexcept { return m_itemCount; }

    size_t countBitsSet() const noexcept;
    double fillRatio() const noexcept;
    double estimatedCurrentFpr() const noexcept;
    double theoreticalFpr() const noexcept { return m_targetFpr; }

    // Serialization support
    const std::vector<uint64_t>& rawBits() const noexcept { return m_bits; }
    void loadRawBits(std::vector<uint64_t> bits, size_t numBits, size_t numHashes, size_t itemCount);


    // Hash helpers
    static uint64_t fnv1a64(std::string_view key) noexcept;
    static uint64_t murmur64(std::string_view key, uint64_t seed) noexcept;

private:
    size_t m_expectedItems{100000};
    double m_targetFpr{0.01};
    size_t m_numBits{0};
    size_t m_numHashes{0};
    size_t m_itemCount{0};

    std::vector<uint64_t> m_bits;
    mutable std::shared_mutex m_mutex;

    void setBit(size_t index) noexcept;
    bool testBit(size_t index) const noexcept;
};

} // namespace crawler

#endif // CRAWLER_BLOOM_FILTER_H
