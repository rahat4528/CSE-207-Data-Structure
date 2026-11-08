#include "StateManager.h"
#include <queue>
#include <algorithm>

namespace DepResolver {

std::vector<std::string> StateManager::findOrphans(const InstalledState& state, const Registry& registry) {
    // 1. Identify all explicitly installed packages
    std::unordered_set<std::string> reachableFromExplicit;
    std::queue<std::string> q;

    for (const auto& [name, installedPkg] : state.getAll()) {
        if (installedPkg.reason == InstallReason::EXPLICIT) {
            reachableFromExplicit.insert(name);
            q.push(name);
        }
    }

    // 2. BFS forward traversal through dependencies of reachable installed packages
    while (!q.empty()) {
        std::string currName = q.front();
        q.pop();

        const auto* installedPkg = state.get(currName);
        if (!installedPkg) continue;

        const auto* pkg = registry.getPackage(currName);
        if (!pkg) continue;

        const auto* pv = pkg->findVersion(installedPkg->version);
        if (!pv) continue;

        for (const auto& dep : pv->dependencies) {
            if (dep.kind == DependencyKind::RUNTIME_DEP) {
                // If the dependency is currently installed and hasn't been visited yet
                if (state.isInstalled(dep.packageName)) {
                    if (reachableFromExplicit.find(dep.packageName) == reachableFromExplicit.end()) {
                        reachableFromExplicit.insert(dep.packageName);
                        q.push(dep.packageName);
                    }
                }
            }
        }
    }

    // 3. Any installed package that is NOT reachable from explicit packages is an orphan!
    std::vector<std::string> orphans;
    for (const auto& [name, installedPkg] : state.getAll()) {
        if (reachableFromExplicit.find(name) == reachableFromExplicit.end()) {
            orphans.push_back(name);
        }
    }

    std::sort(orphans.begin(), orphans.end());
    return orphans;
}

std::vector<std::string> StateManager::findUnused(const InstalledState& state, const Registry& registry) {
    // Collect all packages required by currently installed packages
    std::unordered_set<std::string> activePackages;
    for (const auto& [name, installedPkg] : state.getAll()) {
        activePackages.insert(name);
        const auto* pkg = registry.getPackage(name);
        if (!pkg) continue;
        const auto* pv = pkg->findVersion(installedPkg.version);
        if (!pv) continue;
        for (const auto& dep : pv->dependencies) {
            activePackages.insert(dep.packageName);
        }
    }

    // Unused = in registry but not in activePackages
    std::vector<std::string> unused;
    for (const auto* pkg : registry.getAllPackagesSorted()) {
        if (activePackages.find(pkg->name()) == activePackages.end()) {
            unused.push_back(pkg->name());
        }
    }

    std::sort(unused.begin(), unused.end());
    return unused;
}

std::vector<VersionTransition> StateManager::analyzeTransitions(
    const InstalledState& state, 
    const std::map<std::string, SemVer>& targetResolved) 
{
    std::vector<VersionTransition> transitions;

    for (const auto& [name, targetVer] : targetResolved) {
        VersionTransition trans;
        trans.packageName = name;
        trans.newVersion = targetVer;

        const auto* currentInstalled = state.get(name);
        if (!currentInstalled) {
            trans.type = TransitionType::NEW_INSTALL;
        } else {
            trans.oldVersion = currentInstalled->version;
            int cmp = targetVer.compare(currentInstalled->version);
            if (cmp == 0) {
                trans.type = TransitionType::ALREADY_INSTALLED;
            } else if (cmp > 0) {
                trans.type = TransitionType::UPGRADE;
                if (targetVer.major > currentInstalled->version.major) {
                    trans.isMajorChange = true;
                    trans.warning = "Major version upgrade (" + currentInstalled->version.toString() + 
                                    " -> " + targetVer.toString() + "): potential breaking changes!";
                }
            } else {
                trans.type = TransitionType::DOWNGRADE;
                if (targetVer.major < currentInstalled->version.major) {
                    trans.isMajorChange = true;
                    trans.warning = "Major version downgrade (" + currentInstalled->version.toString() + 
                                    " -> " + targetVer.toString() + "): potential breaking changes!";
                }
            }
        }
        transitions.push_back(std::move(trans));
    }

    return transitions;
}

std::vector<std::string> StateManager::autoRemoveOrphans(InstalledState& state, const Registry& registry) {
    auto orphans = findOrphans(state, registry);
    for (const auto& orphan : orphans) {
        state.recordUninstall(orphan);
    }
    return orphans;
}

} // namespace DepResolver

