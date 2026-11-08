#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <string_view>
#include <cstdint>
#include <iostream>

namespace DepResolver {

struct SemVer {
    uint64_t major{0};
    uint64_t minor{0};
    uint64_t patch{0};
    std::vector<std::string> prerelease;    // e.g. ["alpha", "1"]
    std::vector<std::string> buildMetadata; // e.g. ["build", "123"]

    SemVer() = default;
    SemVer(uint64_t maj, uint64_t min, uint64_t pat, 
           std::vector<std::string> pre = {}, 
           std::vector<std::string> build = {})
        : major(maj), minor(min), patch(pat), 
          prerelease(std::move(pre)), buildMetadata(std::move(build)) {}

    static Result<SemVer> parse(std::string_view sv);

    bool isPrerelease() const { return !prerelease.empty(); }

    std::string toString() const;

    // Strict SemVer 2.0.0 Precedence Comparison
    // Returns < 0 if this < other, 0 if equal precedence, > 0 if this > other
    int compare(const SemVer& other) const;

    bool operator==(const SemVer& other) const { return compare(other) == 0; }
    bool operator!=(const SemVer& other) const { return compare(other) != 0; }
    bool operator<(const SemVer& other) const { return compare(other) < 0; }
    bool operator<=(const SemVer& other) const { return compare(other) <= 0; }
    bool operator>(const SemVer& other) const { return compare(other) > 0; }
    bool operator>=(const SemVer& other) const { return compare(other) >= 0; }
};

inline std::ostream& operator<<(std::ostream& os, const SemVer& v) {
    return os << v.toString();
}

} // namespace DepResolver

