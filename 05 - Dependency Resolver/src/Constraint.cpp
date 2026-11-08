#include "Constraint.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace DepResolver {

// ============================================================================
// VersionInterval Implementation
// ============================================================================

bool VersionInterval::contains(const SemVer& v) const {
    // SemVer 2.0.0 Pre-release rule:
    // If version is pre-release, it matches ONLY if interval explicitly specifies pre-release for same major.minor.patch
    if (v.isPrerelease()) {
        if (!explicitlyAllowsPrerelease) return false;
        if (v.major != prereleaseTarget.major || 
            v.minor != prereleaseTarget.minor || 
            v.patch != prereleaseTarget.patch) {
            return false;
        }
    }

    // Check lower bound
    int lowerCmp = v.compare(minVer);
    if (minInclusive) {
        if (lowerCmp < 0) return false;
    } else {
        if (lowerCmp <= 0) return false;
    }

    // Check upper bound if present
    if (hasMax) {
        int upperCmp = v.compare(maxVer);
        if (maxInclusive) {
            if (upperCmp > 0) return false;
        } else {
            if (upperCmp >= 0) return false;
        }
    }

    return true;
}

std::optional<VersionInterval> VersionInterval::intersect(const VersionInterval& a, const VersionInterval& b) {
    VersionInterval result;

    // Determine new lower bound (maximum of both lower bounds)
    int lowerCmp = a.minVer.compare(b.minVer);
    if (lowerCmp > 0) {
        result.minVer = a.minVer;
        result.minInclusive = a.minInclusive;
    } else if (lowerCmp < 0) {
        result.minVer = b.minVer;
        result.minInclusive = b.minInclusive;
    } else {
        result.minVer = a.minVer;
        result.minInclusive = a.minInclusive && b.minInclusive; // both must be inclusive to include point
    }

    // Determine new upper bound
    if (!a.hasMax && !b.hasMax) {
        result.hasMax = false;
    } else if (a.hasMax && !b.hasMax) {
        result.hasMax = true;
        result.maxVer = a.maxVer;
        result.maxInclusive = a.maxInclusive;
    } else if (!a.hasMax && b.hasMax) {
        result.hasMax = true;
        result.maxVer = b.maxVer;
        result.maxInclusive = b.maxInclusive;
    } else {
        // Both have max: take minimum of both upper bounds
        int upperCmp = a.maxVer.compare(b.maxVer);
        if (upperCmp < 0) {
            result.hasMax = true;
            result.maxVer = a.maxVer;
            result.maxInclusive = a.maxInclusive;
        } else if (upperCmp > 0) {
            result.hasMax = true;
            result.maxVer = b.maxVer;
            result.maxInclusive = b.maxInclusive;
        } else {
            result.hasMax = true;
            result.maxVer = a.maxVer;
            result.maxInclusive = a.maxInclusive && b.maxInclusive;
        }
    }

    // Propagate prerelease targets if any
    if (a.explicitlyAllowsPrerelease || b.explicitlyAllowsPrerelease) {
        result.explicitlyAllowsPrerelease = true;
        result.prereleaseTarget = a.explicitlyAllowsPrerelease ? a.prereleaseTarget : b.prereleaseTarget;
    }

    // Validate that the intersected interval is non-empty
    if (result.hasMax) {
        int spanCmp = result.minVer.compare(result.maxVer);
        if (spanCmp > 0) {
            return std::nullopt; // Empty: min > max
        }
        if (spanCmp == 0 && (!result.minInclusive || !result.maxInclusive)) {
            return std::nullopt; // Empty: min == max, but open endpoint
        }
    }

    return result;
}

std::string VersionInterval::toString() const {
    std::ostringstream ss;
    ss << (minInclusive ? ">=" : ">") << minVer.toString();
    if (hasMax) {
        ss << ", " << (maxInclusive ? "<=" : "<") << maxVer.toString();
    }
    return ss.str();
}

// ============================================================================
// Constraint Parsing Helpers
// ============================================================================

namespace {

std::string trim(std::string_view sv) {
    size_t first = 0;
    while (first < sv.size() && std::isspace(static_cast<unsigned char>(sv[first]))) {
        first++;
    }
    if (first == sv.size()) return "";
    size_t last = sv.size() - 1;
    while (last > first && std::isspace(static_cast<unsigned char>(sv[last]))) {
        last--;
    }
    return std::string(sv.substr(first, last - first + 1));
}

// Parse a single atomic token (e.g. "^1.2.3", ">=2.0.0", "*", "~0.1.0")
Result<VersionInterval> parseAtomicClause(std::string_view token) {
    std::string s = trim(token);
    if (s.empty()) {
        return Result<VersionInterval>::err(ErrorCode::INVALID_CONSTRAINT, "Empty constraint token");
    }

    // Wildcards: "*", "x", "X"
    if (s == "*" || s == "x" || s == "X") {
        VersionInterval iv;
        iv.minVer = SemVer(0, 0, 0);
        iv.minInclusive = true;
        iv.hasMax = false;
        return Result<VersionInterval>::ok(iv);
    }

    // Caret operator: ^1.2.3
    if (s[0] == '^') {
        auto verRes = SemVer::parse(s.substr(1));
        if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
        const auto& v = verRes.value();

        VersionInterval iv;
        iv.minVer = v;
        iv.minInclusive = true;
        iv.hasMax = true;
        iv.maxInclusive = false;
        if (v.isPrerelease()) {
            iv.explicitlyAllowsPrerelease = true;
            iv.prereleaseTarget = v;
        }

        if (v.major > 0) {
            iv.maxVer = SemVer(v.major + 1, 0, 0);
        } else if (v.minor > 0) {
            iv.maxVer = SemVer(0, v.minor + 1, 0);
        } else {
            // ^0.0.x is exact version (patch cannot change)
            iv.maxVer = v;
            iv.maxInclusive = true;
        }
        return Result<VersionInterval>::ok(iv);
    }

    // Tilde operator: ~1.2.3
    if (s[0] == '~') {
        auto verRes = SemVer::parse(s.substr(1));
        if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
        const auto& v = verRes.value();

        VersionInterval iv;
        iv.minVer = v;
        iv.minInclusive = true;
        iv.hasMax = true;
        iv.maxVer = SemVer(v.major, v.minor + 1, 0);
        iv.maxInclusive = false;
        if (v.isPrerelease()) {
            iv.explicitlyAllowsPrerelease = true;
            iv.prereleaseTarget = v;
        }
        return Result<VersionInterval>::ok(iv);
    }

    // Comparison operators: >=, <=, >, <, =, !=
    if (s.rfind(">=", 0) == 0) {
        auto verRes = SemVer::parse(trim(s.substr(2)));
        if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
        VersionInterval iv;
        iv.minVer = verRes.value();
        iv.minInclusive = true;
        iv.hasMax = false;
        if (iv.minVer.isPrerelease()) {
            iv.explicitlyAllowsPrerelease = true;
            iv.prereleaseTarget = iv.minVer;
        }
        return Result<VersionInterval>::ok(iv);
    }
    if (s.rfind("<=", 0) == 0) {
        auto verRes = SemVer::parse(trim(s.substr(2)));
        if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
        VersionInterval iv;
        iv.minVer = SemVer(0, 0, 0);
        iv.minInclusive = true;
        iv.hasMax = true;
        iv.maxVer = verRes.value();
        iv.maxInclusive = true;
        return Result<VersionInterval>::ok(iv);
    }
    if (s.rfind(">", 0) == 0) {
        auto verRes = SemVer::parse(trim(s.substr(1)));
        if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
        VersionInterval iv;
        iv.minVer = verRes.value();
        iv.minInclusive = false;
        iv.hasMax = false;
        if (iv.minVer.isPrerelease()) {
            iv.explicitlyAllowsPrerelease = true;
            iv.prereleaseTarget = iv.minVer;
        }
        return Result<VersionInterval>::ok(iv);
    }
    if (s.rfind("<", 0) == 0) {
        auto verRes = SemVer::parse(trim(s.substr(1)));
        if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
        VersionInterval iv;
        iv.minVer = SemVer(0, 0, 0);
        iv.minInclusive = true;
        iv.hasMax = true;
        iv.maxVer = verRes.value();
        iv.maxInclusive = false;
        return Result<VersionInterval>::ok(iv);
    }

    // Exact operator or bare version: "=1.2.3" or "1.2.3"
    std::string_view exactView = s;
    if (s[0] == '=') exactView = s.substr(1);
    auto verRes = SemVer::parse(trim(exactView));
    if (verRes.isErr()) return Result<VersionInterval>::err(verRes.error());
    const auto& v = verRes.value();

    VersionInterval iv;
    iv.minVer = v;
    iv.minInclusive = true;
    iv.hasMax = true;
    iv.maxVer = v;
    iv.maxInclusive = true;
    if (v.isPrerelease()) {
        iv.explicitlyAllowsPrerelease = true;
        iv.prereleaseTarget = v;
    }
    return Result<VersionInterval>::ok(iv);
}

// Parse an AND clause consisting of comma-separated or space-separated atomic tokens
Result<std::optional<VersionInterval>> parseAndClause(std::string_view clause) {
    // Tokens separated by comma or space
    std::vector<std::string> tokens;
    std::string cur;
    for (size_t i = 0; i < clause.size(); ++i) {
        char c = clause[i];
        if (c == ',') {
            if (!cur.empty()) {
                tokens.push_back(trim(cur));
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) tokens.push_back(trim(cur));

    if (tokens.empty()) {
        return Result<std::optional<VersionInterval>>::err(ErrorCode::INVALID_CONSTRAINT, "Empty clause");
    }

    VersionInterval accumulated;
    accumulated.minVer = SemVer(0, 0, 0);
    accumulated.minInclusive = true;
    accumulated.hasMax = false;

    for (const auto& tok : tokens) {
        if (tok.empty()) continue;
        auto atomRes = parseAtomicClause(tok);
        if (atomRes.isErr()) {
            return Result<std::optional<VersionInterval>>::err(atomRes.error());
        }
        auto inter = VersionInterval::intersect(accumulated, atomRes.value());
        if (!inter.has_value()) {
            // The AND clause itself is mathematically unsatisfiable (empty)
            return Result<std::optional<VersionInterval>>::ok(std::nullopt);
        }
        accumulated = inter.value();
    }

    return Result<std::optional<VersionInterval>>::ok(accumulated);
}

} // namespace

// ============================================================================
// Constraint Implementation
// ============================================================================

Result<Constraint> Constraint::parse(std::string_view expr) {
    std::string s = trim(expr);
    if (s.empty()) {
        return Result<Constraint>::err(ErrorCode::INVALID_CONSTRAINT, "Constraint expression is empty");
    }
    if (s.size() > Limits::MAX_CONSTRAINT_STRING_LENGTH) {
        return Result<Constraint>::err(ErrorCode::INVALID_CONSTRAINT, "Constraint string exceeds maximum allowed length");
    }

    // Split by OR operator "||"
    std::vector<VersionInterval> validIntervals;
    size_t start = 0;
    while (start < s.size()) {
        size_t orPos = s.find("||", start);
        std::string_view part = (orPos == std::string_view::npos) 
            ? s.substr(start) 
            : s.substr(start, orPos - start);

        auto andRes = parseAndClause(part);
        if (andRes.isErr()) {
            return Result<Constraint>::err(andRes.error());
        }
        if (andRes.value().has_value()) {
            validIntervals.push_back(andRes.value().value());
        }

        if (orPos == std::string_view::npos) break;
        start = orPos + 2;
    }

    if (validIntervals.empty()) {
        return Result<Constraint>::err(ErrorCode::INVALID_CONSTRAINT, "Constraint resolves to an empty interval: " + s);
    }

    return Result<Constraint>::ok(Constraint(s, std::move(validIntervals)));
}

bool Constraint::satisfies(const SemVer& v) const {
    for (const auto& iv : intervals_) {
        if (iv.contains(v)) return true;
    }
    return false;
}

bool Constraint::intersects(const Constraint& other) const {
    return intersect(other).has_value();
}

std::optional<Constraint> Constraint::intersect(const Constraint& other) const {
    std::vector<VersionInterval> resultIntervals;

    // Distributive intersection across OR intervals: (A || B) && (C || D) = (A&&C) || (A&&D) || (B&&C) || (B&&D)
    for (const auto& iv1 : intervals_) {
        for (const auto& iv2 : other.intervals_) {
            auto common = VersionInterval::intersect(iv1, iv2);
            if (common.has_value()) {
                resultIntervals.push_back(common.value());
            }
        }
    }

    if (resultIntervals.empty()) {
        return std::nullopt; // Incompatible / Conflict!
    }

    std::string combinedRaw = "(" + rawExpression_ + ") && (" + other.rawExpression_ + ")";
    return Constraint(combinedRaw, std::move(resultIntervals));
}

std::string Constraint::toString() const {
    if (!rawExpression_.empty()) return rawExpression_;
    std::string res;
    for (size_t i = 0; i < intervals_.size(); ++i) {
        res += intervals_[i].toString();
        if (i + 1 < intervals_.size()) res += " || ";
    }
    return res;
}

} // namespace DepResolver

