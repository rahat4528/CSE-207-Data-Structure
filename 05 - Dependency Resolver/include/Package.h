#pragma once

#include "Common.h"
#include "SemVer.h"
#include "Constraint.h"
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

namespace DepResolver {

// ============================================================================
// Dependency Kinds (DD9)
// ============================================================================
enum class DependencyKind {
    RUNTIME_DEP = 0,
    DEVELOPMENT_DEP,
    OPTIONAL_DEP,
    CONFLICT_DEP
};

inline const char* dependencyKindToString(DependencyKind kind) {
    switch (kind) {
        case DependencyKind::RUNTIME_DEP: return "runtime";
        case DependencyKind::DEVELOPMENT_DEP: return "dev";
        case DependencyKind::OPTIONAL_DEP: return "optional";
        case DependencyKind::CONFLICT_DEP: return "conflict";
        default: return "unknown";
    }
}

// ============================================================================
// Single Dependency Declaration
// ============================================================================
struct DependencyDecl {
    std::string packageName;
    Constraint constraint;
    DependencyKind kind{DependencyKind::RUNTIME_DEP};

    DependencyDecl() = default;
    DependencyDecl(std::string name, Constraint c, DependencyKind k = DependencyKind::RUNTIME_DEP)
        : packageName(std::move(name)), constraint(std::move(c)), kind(k) {}
};

// ============================================================================
// Concrete Package Version Entity
// ============================================================================
struct PackageVersion {
    SemVer version;
    std::vector<DependencyDecl> dependencies;
    std::string checksum; // Content hash for reproducibility & lockfiles (E1)
    std::string description;

    PackageVersion() = default;
    PackageVersion(SemVer v, std::vector<DependencyDecl> deps = {}, std::string csum = "", std::string desc = "")
        : version(std::move(v)), dependencies(std::move(deps)), 
          checksum(std::move(csum)), description(std::move(desc)) {}

    // Find dependency by target package name
    const DependencyDecl* findDependency(const std::string& target) const {
        for (const auto& dep : dependencies) {
            if (dep.packageName == target) return &dep;
        }
        return nullptr;
    }
};

// ============================================================================
// Package Entity (Holds all released versions of a package)
// ============================================================================
class Package {
private:
    std::string name_;
    std::string description_;
    std::vector<PackageVersion> versions_; // Kept sorted by SemVer descending for highest-first lookup

public:
    Package() = default;
    explicit Package(std::string name, std::string desc = "")
        : name_(std::move(name)), description_(std::move(desc)) {}

    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    void setDescription(std::string d) { description_ = std::move(d); }

    const std::vector<PackageVersion>& versions() const { return versions_; }

    Result<void> addVersion(PackageVersion pv);
    Result<void> removeVersion(const SemVer& v);

    const PackageVersion* findVersion(const SemVer& v) const;
    PackageVersion* findVersionMut(const SemVer& v);

    // Get versions satisfying constraint, sorted by version descending (newest first)
    std::vector<const PackageVersion*> getMatchingVersions(const Constraint& c, bool newestFirst = true) const;
};

} // namespace DepResolver

