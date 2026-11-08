#pragma once

#include "Common.h"
#include "SemVer.h"
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <iostream>

namespace DepResolver {

// ============================================================================
// Single Continuous Version Interval [minVer, maxVer)
// ============================================================================
struct VersionInterval {
    SemVer minVer{0, 0, 0};
    bool minInclusive{true};
    bool hasMax{false};
    SemVer maxVer{0, 0, 0};
    bool maxInclusive{false};
    bool explicitlyAllowsPrerelease{false};
    SemVer prereleaseTarget{0, 0, 0};

    bool contains(const SemVer& v) const;

    // Mathematical intersection of two intervals
    static std::optional<VersionInterval> intersect(const VersionInterval& a, const VersionInterval& b);

    std::string toString() const;
};

// ============================================================================
// Composite Constraint (Union of Intervals, supporting OR '||' and AND ',')
// ============================================================================
class Constraint {
private:
    std::string rawExpression_;
    std::vector<VersionInterval> intervals_; // Disjoint or union intervals

public:
    Constraint() = default;
    explicit Constraint(std::string raw, std::vector<VersionInterval> intervals = {})
        : rawExpression_(std::move(raw)), intervals_(std::move(intervals)) {}

    static Result<Constraint> parse(std::string_view expr);

    // Any version matching: '*'
    static Constraint any() {
        VersionInterval iv;
        iv.minVer = SemVer(0, 0, 0);
        iv.minInclusive = true;
        iv.hasMax = false;
        return Constraint("*", {iv});
    }

    // Exact version matching
    static Constraint exact(const SemVer& v) {
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
        return Constraint("=" + v.toString(), {iv});
    }

    bool satisfies(const SemVer& v) const;

    // Check if this constraint has any overlap with another constraint
    bool intersects(const Constraint& other) const;

    // Intersect two constraints: returns std::nullopt if intersection is empty (Conflict!)
    std::optional<Constraint> intersect(const Constraint& other) const;

    const std::string& raw() const { return rawExpression_; }
    const std::vector<VersionInterval>& intervals() const { return intervals_; }

    std::string toString() const;
};

inline std::ostream& operator<<(std::ostream& os, const Constraint& c) {
    return os << c.toString();
}

} // namespace DepResolver

