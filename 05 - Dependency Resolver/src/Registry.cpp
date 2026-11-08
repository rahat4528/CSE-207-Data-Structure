#include "Registry.h"
#include <algorithm>

namespace DepResolver {

Result<void> Registry::registerPackage(std::string name, std::string description) {
    auto nameRes = Validator::validatePackageName(name);
    if (nameRes.isErr()) return Result<void>::err(nameRes.error());
    std::string normalized = nameRes.value();

    if (packages_.find(normalized) != packages_.end()) {
        return Result<void>::err(ErrorCode::ALREADY_EXISTS, "Package already registered: " + normalized, normalized);
    }
    if (packages_.size() >= Limits::MAX_PACKAGES_IN_REGISTRY) {
        return Result<void>::err(ErrorCode::LIMIT_REACHED, "Maximum registry package count exceeded");
    }

    packages_.emplace(normalized, Package(normalized, std::move(description)));
    return Result<void>::ok();
}

Result<void> Registry::registerVersion(const std::string& name, PackageVersion pv) {
    auto nameRes = Validator::validatePackageName(name);
    if (nameRes.isErr()) return Result<void>::err(nameRes.error());
    std::string normalized = nameRes.value();

    auto it = packages_.find(normalized);
    if (it == packages_.end()) {
        // Auto-register package container if not already registered
        auto regRes = registerPackage(normalized);
        if (regRes.isErr()) return regRes;
        it = packages_.find(normalized);
    }

    return it->second.addVersion(std::move(pv));
}

Result<void> Registry::updateDescription(const std::string& name, std::string newDesc) {
    auto it = packages_.find(name);
    if (it == packages_.end()) {
        return Result<void>::err(ErrorCode::PACKAGE_NOT_FOUND, "Package not found: " + name, name);
    }
    it->second.setDescription(std::move(newDesc));
    return Result<void>::ok();
}

std::vector<std::string> Registry::getDirectDependents(const std::string& targetPkg) const {
    std::vector<std::string> dependents;
    for (const auto& [pkgName, pkg] : packages_) {
        if (pkgName == targetPkg) continue;
        for (const auto& pv : pkg.versions()) {
            if (pv.findDependency(targetPkg) != nullptr) {
                dependents.push_back(pkgName);
                break; // already noted this dependent package
            }
        }
    }
    std::sort(dependents.begin(), dependents.end());
    return dependents;
}

Result<void> Registry::removePackage(const std::string& name, bool cascade) {
    auto it = packages_.find(name);
    if (it == packages_.end()) {
        return Result<void>::err(ErrorCode::PACKAGE_NOT_FOUND, "Package not found: " + name, name);
    }

    if (!cascade) {
        auto dependents = getDirectDependents(name);
        if (!dependents.empty()) {
            std::string depList;
            for (size_t i = 0; i < dependents.size(); ++i) {
                depList += dependents[i];
                if (i + 1 < dependents.size()) depList += ", ";
            }
            return Result<void>::err(
                ErrorCode::DEPENDENT_EXISTS,
                "Cannot remove package '" + name + "': it is required by [" + depList + "]. Use --cascade to force.",
                name
            );
        }
    } else {
        // Cascade: remove dependency references from all other packages
        for (auto& [pkgName, pkg] : packages_) {
            for (auto& pv : const_cast<std::vector<PackageVersion>&>(pkg.versions())) {
                auto& deps = pv.dependencies;
                deps.erase(std::remove_if(deps.begin(), deps.end(), [&](const DependencyDecl& d) {
                    return d.packageName == name;
                }), deps.end());
            }
        }
    }

    packages_.erase(it);
    return Result<void>::ok();
}

Result<void> Registry::removeVersion(const std::string& name, const SemVer& v) {
    auto it = packages_.find(name);
    if (it == packages_.end()) {
        return Result<void>::err(ErrorCode::PACKAGE_NOT_FOUND, "Package not found: " + name, name);
    }
    return it->second.removeVersion(v);
}

bool Registry::hasPackage(const std::string& name) const {
    return packages_.find(name) != packages_.end();
}

const Package* Registry::getPackage(const std::string& name) const {
    auto it = packages_.find(name);
    if (it != packages_.end()) return &it->second;
    return nullptr;
}

Package* Registry::getPackageMut(const std::string& name) {
    auto it = packages_.find(name);
    if (it != packages_.end()) return &it->second;
    return nullptr;
}

std::vector<const Package*> Registry::search(std::string_view query) const {
    std::vector<const Package*> results;
    for (const auto& [name, pkg] : packages_) {
        if (name.find(query) != std::string::npos || pkg.description().find(query) != std::string::npos) {
            results.push_back(&pkg);
        }
    }
    std::sort(results.begin(), results.end(), [](const Package* a, const Package* b) {
        return a->name() < b->name();
    });
    return results;
}

std::vector<const Package*> Registry::getAllPackagesSorted() const {
    std::vector<const Package*> list;
    list.reserve(packages_.size());
    for (const auto& [name, pkg] : packages_) {
        list.push_back(&pkg);
    }
    std::sort(list.begin(), list.end(), [](const Package* a, const Package* b) {
        return a->name() < b->name();
    });
    return list;
}

} // namespace DepResolver

