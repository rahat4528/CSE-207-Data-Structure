#pragma once

#include "Common.h"
#include "Package.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <optional>

namespace DepResolver {

class Registry {
private:
    std::unordered_map<std::string, Package> packages_;

public:
    Registry() = default;

    // Package CRUD
    Result<void> registerPackage(std::string name, std::string description = "");
    Result<void> registerVersion(const std::string& name, PackageVersion pv);
    Result<void> updateDescription(const std::string& name, std::string newDesc);

    // Removal with Dependent Checks (DD6)
    Result<void> removePackage(const std::string& name, bool cascade = false);
    Result<void> removeVersion(const std::string& name, const SemVer& v);

    // Queries
    bool hasPackage(const std::string& name) const;
    const Package* getPackage(const std::string& name) const;
    Package* getPackageMut(const std::string& name);

    // Dependents analysis: which packages depend on target package?
    std::vector<std::string> getDirectDependents(const std::string& targetPkg) const;

    // Search
    std::vector<const Package*> search(std::string_view query) const;
    std::vector<const Package*> getAllPackagesSorted() const;

    size_t packageCount() const { return packages_.size(); }
    void clear() { packages_.clear(); }
};

} // namespace DepResolver

