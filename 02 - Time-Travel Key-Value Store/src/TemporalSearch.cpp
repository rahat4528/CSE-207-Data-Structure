#include "TemporalSearch.h"

namespace ttkv {

int TemporalSearch::binarySearchUpperBound(const std::vector<VersionRecord>& versions, uint64_t targetTime) {
    if (versions.empty()) return -1;
    if (targetTime < versions.front().timestamp()) return -1;
    if (targetTime >= versions.back().timestamp()) {
        return static_cast<int>(versions.size()) - 1;
    }

    int low = 0;
    int high = static_cast<int>(versions.size()) - 1;
    int best = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (versions[static_cast<size_t>(mid)].timestamp() <= targetTime) {
            best = mid;
            low = mid + 1; // Move right to find latest matching timestamp / highest sequence ID
        } else {
            high = mid - 1;
        }
    }

    return best;
}

QueryResult TemporalSearch::queryByTimestamp(const KeyRecord& keyRec, uint64_t targetTime) {
    if (keyRec.versions.empty()) {
        return QueryResult::statusOnly(QueryStatus::NOT_YET_EXISTS, keyRec.key);
    }

    int idx = binarySearchUpperBound(keyRec.versions, targetTime);
    if (idx < 0) {
        return QueryResult::statusOnly(QueryStatus::NOT_YET_EXISTS, keyRec.key);
    }

    const auto& v = keyRec.versions[static_cast<size_t>(idx)];
    if (v.isDeleted()) {
        return QueryResult::deleted(v);
    }
    if (v.isExpiredAt(targetTime)) {
        return QueryResult::expired(v);
    }
    return QueryResult::found(v);
}

QueryResult TemporalSearch::queryByVersionNumber(const KeyRecord& keyRec, uint32_t versionNumber) {
    if (versionNumber == 0) {
        return QueryResult::statusOnly(QueryStatus::INVALID_VERSION, keyRec.key);
    }
    if (keyRec.versions.empty()) {
        return QueryResult::statusOnly(QueryStatus::KEY_UNKNOWN, keyRec.key);
    }

    uint32_t firstVer = keyRec.versions.front().keyVersionNumber();
    uint32_t lastVer = keyRec.versions.back().keyVersionNumber();

    if (versionNumber < firstVer) {
        // Pruned
        return QueryResult::statusOnly(QueryStatus::PRUNED, keyRec.key);
    }
    if (versionNumber > lastVer) {
        return QueryResult::statusOnly(QueryStatus::INVALID_VERSION, keyRec.key);
    }

    // Direct O(1) index calculation
    size_t idx = static_cast<size_t>(versionNumber - firstVer);
    if (idx < keyRec.versions.size()) {
        const auto& v = keyRec.versions[idx];
        if (v.keyVersionNumber() == versionNumber) {
            if (v.isDeleted()) {
                return QueryResult::deleted(v);
            }
            return QueryResult::found(v);
        }
    }

    return QueryResult::statusOnly(QueryStatus::INVALID_VERSION, keyRec.key);
}

QueryResult TemporalSearch::queryCurrent(const KeyRecord& keyRec) {
    if (keyRec.versions.empty()) {
        return QueryResult::statusOnly(QueryStatus::KEY_UNKNOWN, keyRec.key);
    }

    const auto& v = keyRec.versions.back();
    if (v.isDeleted()) {
        return QueryResult::deleted(v);
    }
    return QueryResult::found(v);
}

std::pair<int, int> TemporalSearch::findRangeExclusiveStart(const std::vector<VersionRecord>& versions,
                                                           uint64_t startTime,
                                                           uint64_t endTime) {
    if (versions.empty() || startTime >= endTime) {
        return {-1, -1};
    }

    // First version with timestamp > startTime
    int low = 0;
    int high = static_cast<int>(versions.size()) - 1;
    int startIdx = -1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (versions[static_cast<size_t>(mid)].timestamp() > startTime) {
            startIdx = mid;
            high = mid - 1; // Move left to find first
        } else {
            low = mid + 1;
        }
    }

    if (startIdx == -1) return {-1, -1};

    // Last version with timestamp <= endTime
    int endIdx = binarySearchUpperBound(versions, endTime);
    if (endIdx < startIdx) return {-1, -1};

    return {startIdx, endIdx};
}

} // namespace ttkv
