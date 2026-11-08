#pragma once

#include "Common.h"
#include "SemVer.h"
#include "Registry.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace DepResolver {

enum class InstallReason {
    EXPLICIT = 0,
    DEPENDENCY
};

inline const char* installReasonToString(InstallReason r) {
    switch (r) {
        case InstallReason::EXPLICIT: return "explicit";
        case InstallReason::DEPENDENCY: return "dependency";
        default: return "unknown";
    }
}

struct InstalledPackage {
    std::string name;
    SemVer version;
    InstallReason reason{InstallReason::EXPLICIT};
    std::string installedAt;
    uint64_t transactionId{0};

    InstalledPackage() = default;
    InstalledPackage(std::string n, SemVer v, InstallReason r, std::string timestamp = "", uint64_t tx = 0)
        : name(std::move(n)), version(std::move(v)), reason(r), 
          installedAt(std::move(timestamp)), transactionId(tx) {}
};

class InstalledState {
private:
    std::unordered_map<std::string, InstalledPackage> packages_;

public:
    InstalledState() = default;

    bool isInstalled(const std::string& name) const {
        return packages_.find(name) != packages_.end();
    }

    const InstalledPackage* get(const std::string& name) const {
        auto it = packages_.find(name);
        if (it != packages_.end()) return &it->second;
        return nullptr;
    }

    void recordInstall(InstalledPackage pkg) {
        packages_[pkg.name] = std::move(pkg);
    }

    bool recordUninstall(const std::string& name) {
        return packages_.erase(name) > 0;
    }

    const std::unordered_map<std::string, InstalledPackage>& getAll() const {
        return packages_;
    }

    size_t count() const { return packages_.size(); }
    void clear() { packages_.clear(); }
};

enum class TransitionType {
    NEW_INSTALL = 0,
    UPGRADE,
    DOWNGRADE,
    ALREADY_INSTALLED,
    UNINSTALL
};

inline const char* transitionTypeToString(TransitionType t) {
    switch (t) {
        case TransitionType::NEW_INSTALL: return "NEW_INSTALL";
        case TransitionType::UPGRADE: return "UPGRADE";
        case TransitionType::DOWNGRADE: return "DOWNGRADE";
        case TransitionType::ALREADY_INSTALLED: return "ALREADY_INSTALLED";
        case TransitionType::UNINSTALL: return "UNINSTALL";
        default: return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, TransitionType t) {
    return os << transitionTypeToString(t);
}

struct VersionTransition {
    std::string packageName;
    TransitionType type{TransitionType::NEW_INSTALL};
    std::optional<SemVer> oldVersion;
    SemVer newVersion;
    bool isMajorChange{false};
    std::string warning;
};

class StateManager {
public:
    // O(V + E) Reachability to detect packages installed only as dependencies
    // that are no longer reachable from any explicitly requested root package
    static std::vector<std::string> findOrphans(const InstalledState& state, const Registry& registry);

    // O(V + E) Reachability to find registered packages that are neither installed
    // nor required by any installed package
    static std::vector<std::string> findUnused(const InstalledState& state, const Registry& registry);

    // Analyze transitions (New install, upgrade, downgrade, major change warnings)
    static std::vector<VersionTransition> analyzeTransitions(
        const InstalledState& state, 
        const std::map<std::string, SemVer>& targetResolved
    );

    // Auto-remove orphans
    static std::vector<std::string> autoRemoveOrphans(InstalledState& state, const Registry& registry);
};

} // namespace DepResolver
