#pragma once

#include "Common.h"
#include "DataModel.h"
#include <vector>
#include <cstdint>
#include <optional>

namespace ttkv {

class TemporalSearch {
public:
    // Finds the effective version at a specified timestamp using O(log V) binary search.
    // Returns the latest version whose timestamp <= targetTime.
    // Handles identical timestamps by selecting the highest sequence number.
    static QueryResult queryByTimestamp(const KeyRecord& keyRec, uint64_t targetTime);

    // Finds the version by per-key version number (1, 2, ...) using O(1) direct access.
    static QueryResult queryByVersionNumber(const KeyRecord& keyRec, uint32_t versionNumber);

    // Returns the current (latest) version of the key.
    static QueryResult queryCurrent(const KeyRecord& keyRec);

    // Raw binary search returning index in versions vector, or -1 if targetTime < first version
    static int binarySearchUpperBound(const std::vector<VersionRecord>& versions, uint64_t targetTime);

    // Finds range of versions with timestamp in (startTime, endTime]
    static std::pair<int, int> findRangeExclusiveStart(const std::vector<VersionRecord>& versions,
                                                       uint64_t startTime,
                                                       uint64_t endTime);
};

} // namespace ttkv
