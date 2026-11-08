#include "ContentHasher.h"
#include "BloomFilter.h"
#include "HtmlParser.h"
#include <bit>
#include <cctype>

namespace crawler {

uint64_t ContentHasher::computeExactHash(std::string_view cleanText) noexcept {
    std::string normalized = HtmlParser::collapseWhitespace(cleanText);
    // 64-bit FNV-1a hash
    uint64_t h = 14695981039346656037ULL;
    for (char c : normalized) {
        h ^= static_cast<uint8_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}


std::vector<std::string> ContentHasher::tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string current;

    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

uint64_t ContentHasher::computeSimHash(std::string_view cleanText) {
    auto tokens = tokenize(cleanText);
    if (tokens.empty()) return 0;

    int v[64] = {0};

    for (const auto& token : tokens) {
        uint64_t h = BloomFilter::fnv1a64(token);
        for (int i = 0; i < 64; ++i) {
            if ((h & (1ULL << i)) != 0) {
                v[i] += 1;
            } else {
                v[i] -= 1;
            }
        }
    }

    uint64_t fingerprint = 0;
    for (int i = 0; i < 64; ++i) {
        if (v[i] > 0) {
            fingerprint |= (1ULL << i);
        }
    }
    return fingerprint;
}

uint32_t ContentHasher::hammingDistance(uint64_t a, uint64_t b) noexcept {
    return static_cast<uint32_t>(std::popcount(a ^ b));
}

bool ContentHasher::isNearDuplicate(uint64_t a, uint64_t b, uint32_t maxDistance) noexcept {
    return hammingDistance(a, b) <= maxDistance;
}

ContentFingerprint ContentHasher::compute(std::string_view cleanText) {
    ContentFingerprint fp;
    fp.exactHash64 = computeExactHash(cleanText);
    fp.simHash64 = computeSimHash(cleanText);
    fp.contentLength = cleanText.size();
    return fp;
}

} // namespace crawler
