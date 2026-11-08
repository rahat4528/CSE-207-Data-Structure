#ifndef CRAWLER_CONTENT_HASHER_H
#define CRAWLER_CONTENT_HASHER_H

#include "Common.h"
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

namespace crawler {

struct ContentFingerprint {
    uint64_t exactHash64{0};
    uint64_t simHash64{0};
    size_t contentLength{0};
};

class ContentHasher {
public:
    static ContentFingerprint compute(std::string_view cleanText);

    static uint64_t computeExactHash(std::string_view cleanText) noexcept;
    static uint64_t computeSimHash(std::string_view cleanText);

    // Hamming distance between two 64-bit SimHash values
    static uint32_t hammingDistance(uint64_t a, uint64_t b) noexcept;

    // Checks if two documents are near-duplicates (default distance <= 3)
    static bool isNearDuplicate(uint64_t a, uint64_t b, uint32_t maxDistance = 3) noexcept;

private:
    static std::vector<std::string> tokenize(std::string_view text);
};

} // namespace crawler

#endif // CRAWLER_CONTENT_HASHER_H

