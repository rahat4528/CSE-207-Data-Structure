#include "SemVer.h"
#include <sstream>
#include <cctype>
#include <limits>

namespace DepResolver {

namespace {

bool isAllDigits(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

// Strict numeric parse: rejects leading zeros like "01" (unless string is exactly "0")
bool parseNumericPart(std::string_view s, uint64_t& out) {
    if (s.empty()) return false;
    if (!isAllDigits(s)) return false;
    if (s.size() > 1 && s[0] == '0') return false; // Leading zero violation

    uint64_t val = 0;
    for (char c : s) {
        uint64_t digit = static_cast<uint64_t>(c - '0');
        if (val > (std::numeric_limits<uint64_t>::max() - digit) / 10) {
            return false; // Overflow
        }
        val = val * 10 + digit;
    }
    out = val;
    return true;
}

std::vector<std::string> splitByDot(std::string_view s) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start < s.size()) {
        size_t dotPos = s.find('.', start);
        if (dotPos == std::string_view::npos) {
            parts.emplace_back(s.substr(start));
            break;
        }
        parts.emplace_back(s.substr(start, dotPos - start));
        start = dotPos + 1;
    }
    return parts;
}

} // namespace

Result<SemVer> SemVer::parse(std::string_view sv) {
    if (sv.empty()) {
        return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Version string is empty");
    }
    if (sv.size() > Limits::MAX_VERSION_STRING_LENGTH) {
        return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Version string exceeds maximum length");
    }

    // Optional leading 'v' or 'V' (common in git tags, normalize by stripping)
    if (sv[0] == 'v' || sv[0] == 'V') {
        sv.remove_prefix(1);
    }
    if (sv.empty()) {
        return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Version string contains only 'v'");
    }

    // Split build metadata (+...)
    std::string_view mainAndPre = sv;
    std::string_view buildView;
    size_t plusPos = sv.find('+');
    if (plusPos != std::string_view::npos) {
        mainAndPre = sv.substr(0, plusPos);
        buildView = sv.substr(plusPos + 1);
        if (buildView.empty()) {
            return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Empty build metadata after '+'");
        }
    }

    // Split pre-release (-...)
    std::string_view mainView = mainAndPre;
    std::string_view preView;
    size_t dashPos = mainAndPre.find('-');
    if (dashPos != std::string_view::npos) {
        mainView = mainAndPre.substr(0, dashPos);
        preView = mainAndPre.substr(dashPos + 1);
        if (preView.empty()) {
            return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Empty pre-release identifier after '-'");
        }
    }

    // Main view must be MAJOR.MINOR.PATCH
    auto coreParts = splitByDot(mainView);
    if (coreParts.size() != 3) {
        return Result<SemVer>::err(
            ErrorCode::INVALID_VERSION_FORMAT,
            "SemVer core must contain exactly three parts: MAJOR.MINOR.PATCH (got " + std::to_string(coreParts.size()) + ")"
        );
    }

    uint64_t maj = 0, min = 0, pat = 0;
    if (!parseNumericPart(coreParts[0], maj)) {
        return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Invalid or overflowed MAJOR version: " + coreParts[0]);
    }
    if (!parseNumericPart(coreParts[1], min)) {
        return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Invalid or overflowed MINOR version: " + coreParts[1]);
    }
    if (!parseNumericPart(coreParts[2], pat)) {
        return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Invalid or overflowed PATCH version: " + coreParts[2]);
    }

    // Parse pre-release identifiers
    std::vector<std::string> preIdents;
    if (!preView.empty()) {
        auto rawPre = splitByDot(preView);
        for (const auto& ident : rawPre) {
            if (ident.empty()) {
                return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Empty pre-release identifier segment");
            }
            // Check character validity: [0-9A-Za-z-]
            for (char c : ident) {
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
                    return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, std::string("Illegal character in pre-release: '") + c + "'");
                }
            }
            // If numeric, cannot have leading zero
            if (isAllDigits(ident) && ident.size() > 1 && ident[0] == '0') {
                return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Numeric pre-release identifier cannot have leading zero: " + ident);
            }
            preIdents.push_back(ident);
        }
    }

    // Parse build metadata identifiers
    std::vector<std::string> buildIdents;
    if (!buildView.empty()) {
        auto rawBuild = splitByDot(buildView);
        for (const auto& ident : rawBuild) {
            if (ident.empty()) {
                return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, "Empty build metadata segment");
            }
            for (char c : ident) {
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
                    return Result<SemVer>::err(ErrorCode::INVALID_VERSION_FORMAT, std::string("Illegal character in build metadata: '") + c + "'");
                }
            }
            buildIdents.push_back(ident);
        }
    }

    return Result<SemVer>::ok(SemVer(maj, min, pat, std::move(preIdents), std::move(buildIdents)));
}

std::string SemVer::toString() const {
    std::ostringstream ss;
    ss << major << "." << minor << "." << patch;
    if (!prerelease.empty()) {
        ss << "-";
        for (size_t i = 0; i < prerelease.size(); ++i) {
            ss << prerelease[i];
            if (i + 1 < prerelease.size()) ss << ".";
        }
    }
    if (!buildMetadata.empty()) {
        ss << "+";
        for (size_t i = 0; i < buildMetadata.size(); ++i) {
            ss << buildMetadata[i];
            if (i + 1 < buildMetadata.size()) ss << ".";
        }
    }
    return ss.str();
}

int SemVer::compare(const SemVer& other) const {
    // 1. Compare major, minor, patch numerically
    if (major != other.major) return major < other.major ? -1 : 1;
    if (minor != other.minor) return minor < other.minor ? -1 : 1;
    if (patch != other.patch) return patch < other.patch ? -1 : 1;

    // 2. A version with prerelease has LOWER precedence than a version without
    if (prerelease.empty() && !other.prerelease.empty()) return 1;  // normal > prerelease
    if (!prerelease.empty() && other.prerelease.empty()) return -1; // prerelease < normal
    if (prerelease.empty() && other.prerelease.empty()) return 0;   // both normal, equal

    // 3. Both have prerelease: compare identifier by identifier
    size_t minLen = std::min(prerelease.size(), other.prerelease.size());
    for (size_t i = 0; i < minLen; ++i) {
        const std::string& a = prerelease[i];
        const std::string& b = other.prerelease[i];

        bool aNum = isAllDigits(a);
        bool bNum = isAllDigits(b);

        if (aNum && bNum) {
            uint64_t aVal = std::stoull(a);
            uint64_t bVal = std::stoull(b);
            if (aVal != bVal) return aVal < bVal ? -1 : 1;
        } else if (aNum && !bNum) {
            // Numeric identifiers always have lower precedence than non-numeric
            return -1;
        } else if (!aNum && bNum) {
            return 1;
        } else {
            // Both are lexical: ASCII sort order
            if (a != b) return a < b ? -1 : 1;
        }
    }

    // 4. A larger set of pre-release fields has a higher precedence if shared are equal
    if (prerelease.size() != other.prerelease.size()) {
        return prerelease.size() < other.prerelease.size() ? -1 : 1;
    }

    // Build metadata is ignored in precedence comparison
    return 0;
}

} // namespace DepResolver

