#include "Package.h"

namespace DepResolver {

Result<void> Package::addVersion(PackageVersion pv) {
    if (versions_.size() >= Limits::MAX_VERSIONS_PER_PACKAGE) {
        return Result<void>::err(ErrorCode::LIMIT_REACHED, "Maximum versions limit reached for package: " + name_);
    }

    // Check for duplicate version
    for (const auto& existing : versions_) {
        if (existing.version == pv.version) {
            return Result<void>::err(
                ErrorCode::ALREADY_EXISTS,
                "Version " + pv.version.toString() + " already exists in package " + name_
            );
        }
    }

    versions_.push_back(std::move(pv));

    // Sort descending by version (highest version first)
    std::sort(versions_.begin(), versions_.end(), [](const PackageVersion& a, const PackageVersion& b) {
        return a.version > b.version;
    });

    return Result<void>::ok();
}

Result<void> Package::removeVersion(const SemVer& v) {
    auto it = std::find_if(versions_.begin(), versions_.end(), [&](const PackageVersion& pv) {
        return pv.version == v;
    });

    if (it == versions_.end()) {
        return Result<void>::err(
            ErrorCode::VERSION_NOT_FOUND,
            "Version " + v.toString() + " not found in package " + name_
        );
    }

    versions_.erase(it);
    return Result<void>::ok();
}

const PackageVersion* Package::findVersion(const SemVer& v) const {
    for (const auto& pv : versions_) {
        if (pv.version == v) return &pv;
    }
    return nullptr;
}

PackageVersion* Package::findVersionMut(const SemVer& v) {
    for (auto& pv : versions_) {
        if (pv.version == v) return &pv;
    }
    return nullptr;
}

std::vector<const PackageVersion*> Package::getMatchingVersions(const Constraint& c, bool newestFirst) const {
    std::vector<const PackageVersion*> matches;
    for (const auto& pv : versions_) {
        if (c.satisfies(pv.version)) {
            matches.push_back(&pv);
        }
    }

    // matches are already descending because versions_ is sorted descending
    if (!newestFirst) {
        std::reverse(matches.begin(), matches.end());
    }

    return matches;
}

} // namespace DepResolver

