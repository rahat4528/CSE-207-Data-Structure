#include "BloomFilter.h"
#include <algorithm>
#include <bit>

namespace crawler {

uint64_t BloomFilter::fnv1a64(std::string_view key) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : key) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

uint64_t BloomFilter::murmur64(std::string_view key, uint64_t seed) noexcept {
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    uint64_t h = seed ^ (key.size() * m);

    const uint64_t* data = reinterpret_cast<const uint64_t*>(key.data());
    const uint64_t* end = data + (key.size() / 8);

    while (data != end) {
        uint64_t k = *data++;
        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const uint8_t* data2 = reinterpret_cast<const uint8_t*>(data);
    switch (key.size() & 7) {
        case 7: h ^= static_cast<uint64_t>(data2[6]) << 48; [[fallthrough]];
        case 6: h ^= static_cast<uint64_t>(data2[5]) << 40; [[fallthrough]];
        case 5: h ^= static_cast<uint64_t>(data2[4]) << 32; [[fallthrough]];
        case 4: h ^= static_cast<uint64_t>(data2[3]) << 24; [[fallthrough]];
        case 3: h ^= static_cast<uint64_t>(data2[2]) << 16; [[fallthrough]];
        case 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  [[fallthrough]];
        case 1: h ^= static_cast<uint64_t>(data2[0]);
                h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

BloomFilter::BloomFilter(size_t expectedItems, double targetFpr)
    : m_expectedItems(std::max<size_t>(expectedItems, 1)),
      m_targetFpr((targetFpr <= 0.0 || targetFpr >= 1.0) ? 0.01 : targetFpr) {
    // Sizing formula: m = -n * ln(p) / (ln 2)^2
    double ln2 = 0.6931471805599453;
    double m = -static_cast<double>(m_expectedItems) * std::log(m_targetFpr) / (ln2 * ln2);
    m_numBits = std::max<size_t>(static_cast<size_t>(std::ceil(m)), 64);

    // Number of hashes: k = (m / n) * ln 2
    double k = (static_cast<double>(m_numBits) / static_cast<double>(m_expectedItems)) * ln2;
    m_numHashes = std::max<size_t>(static_cast<size_t>(std::round(k)), 1);

    size_t words = (m_numBits + 63) / 64;
    m_bits.assign(words, 0);
}

BloomFilter::BloomFilter(size_t numBits, size_t numHashes)
    : m_numBits(std::max<size_t>(numBits, 64)),
      m_numHashes(std::max<size_t>(numHashes, 1)) {
    size_t words = (m_numBits + 63) / 64;
    m_bits.assign(words, 0);
}

void BloomFilter::setBit(size_t index) noexcept {
    size_t wordIdx = index / 64;
    size_t bitIdx = index % 64;
    m_bits[wordIdx] |= (1ULL << bitIdx);
}

bool BloomFilter::testBit(size_t index) const noexcept {
    size_t wordIdx = index / 64;
    size_t bitIdx = index % 64;
    return (m_bits[wordIdx] & (1ULL << bitIdx)) != 0;
}

void BloomFilter::insert(std::string_view key) {
    uint64_t h1 = fnv1a64(key);
    uint64_t h2 = murmur64(key, 0x9747b28cULL);

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (size_t i = 0; i < m_numHashes; ++i) {
        // Kirsch-Mitzenmacher: gi(x) = h1(x) + i * h2(x)
        size_t bitPos = static_cast<size_t>((h1 + i * h2) % m_numBits);
        setBit(bitPos);
    }
    m_itemCount++;
}

bool BloomFilter::possiblyContains(std::string_view key) const {
    uint64_t h1 = fnv1a64(key);
    uint64_t h2 = murmur64(key, 0x9747b28cULL);

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (size_t i = 0; i < m_numHashes; ++i) {
        size_t bitPos = static_cast<size_t>((h1 + i * h2) % m_numBits);
        if (!testBit(bitPos)) {
            return false;
        }
    }
    return true;
}

size_t BloomFilter::countBitsSet() const noexcept {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    size_t setBits = 0;
    for (uint64_t word : m_bits) {
        setBits += static_cast<size_t>(std::popcount(word));
    }
    return setBits;
}

double BloomFilter::fillRatio() const noexcept {
    if (m_numBits == 0) return 0.0;
    return static_cast<double>(countBitsSet()) / static_cast<double>(m_numBits);
}

double BloomFilter::estimatedCurrentFpr() const noexcept {
    if (m_numBits == 0 || m_expectedItems == 0) return 0.0;
    // FPR = (1 - e^(-k * n / m))^k
    double exponent = -static_cast<double>(m_numHashes) * static_cast<double>(m_itemCount) / static_cast<double>(m_numBits);
    double base = 1.0 - std::exp(exponent);
    return std::pow(base, static_cast<double>(m_numHashes));
}

void BloomFilter::loadRawBits(std::vector<uint64_t> bits, size_t numBits, size_t numHashes, size_t itemCount) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_bits = std::move(bits);
    m_numBits = numBits;
    m_numHashes = numHashes;
    m_itemCount = itemCount;
}


} // namespace crawler
