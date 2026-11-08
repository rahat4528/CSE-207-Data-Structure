#include "SnapshotEngine.h"
#include "Timestamp.h"
#include "Validation.h"
#include <algorithm>

namespace ttkv {

SnapshotEngine::SnapshotEngine() = default;

Result<KeyDiff> SnapshotEngine::diffKey(const KeyRecord& keyRec, uint64_t timeA, uint64_t timeB) {
    if (timeA > timeB) {
        return Result<KeyDiff>::error(ErrorCode::ERR_INVALID_TIMESTAMP,
                                      "Start timestamp TIME_A cannot be later than end timestamp TIME_B");
    }

    QueryResult stateA = TemporalSearch::queryByTimestamp(keyRec, timeA);
    QueryResult stateB = TemporalSearch::queryByTimestamp(keyRec, timeB);

    KeyDiff diff;
    diff.key = keyRec.key;
    diff.stateA = stateA;
    diff.stateB = stateB;

    bool existsA = (stateA.status == QueryStatus::FOUND);
    bool existsB = (stateB.status == QueryStatus::FOUND);

    if (!existsA && existsB) {
        diff.changeType = "ADDED";
    } else if (existsA && !existsB) {
        diff.changeType = "DELETED";
    } else if (existsA && existsB) {
        if (stateA.value == stateB.value) {
            diff.changeType = "UNCHANGED";
        } else {
            diff.changeType = "MODIFIED";
        }
    } else {
        diff.changeType = "NOT_EXISTENT";
    }

    return Result<KeyDiff>::ok(diff);
}

Result<StoreDiff> SnapshotEngine::diffStore(const KeyManager& km, uint64_t timeA, uint64_t timeB) {
    if (timeA > timeB) {
        return Result<StoreDiff>::error(ErrorCode::ERR_INVALID_TIMESTAMP,
                                       "Start timestamp TIME_A cannot be later than end timestamp TIME_B");
    }

    StoreDiff storeDiff;
    storeDiff.timeA = timeA;
    storeDiff.timeB = timeB;

    auto allKeys = km.getAllKeys(true);
    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        auto keyDiffRes = diffKey(*rec, timeA, timeB);
        if (keyDiffRes.isError()) continue;

        const auto& kd = keyDiffRes.value();
        if (kd.changeType == "ADDED") {
            storeDiff.added.push_back(kd);
        } else if (kd.changeType == "MODIFIED") {
            storeDiff.modified.push_back(kd);
        } else if (kd.changeType == "DELETED") {
            storeDiff.deleted.push_back(kd);
        } else if (kd.changeType == "UNCHANGED") {
            storeDiff.unchanged.push_back(kd);
        }
    }

    return Result<StoreDiff>::ok(storeDiff);
}

TimeSnapshot SnapshotEngine::takeTimeSnapshot(const KeyManager& km, uint64_t targetTime) {
    TimeSnapshot snapshot;
    snapshot.timestamp = targetTime;
    snapshot.formattedTime = Timestamp::formatIso8601(targetTime);

    auto allKeys = km.getAllKeys(true);
    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        QueryResult q = TemporalSearch::queryByTimestamp(*rec, targetTime);
        if (q.status == QueryStatus::FOUND) {
            snapshot.activeEntries.push_back(std::move(q));
        }
    }

    return snapshot;
}

std::vector<std::string> SnapshotEngine::searchKeys(const KeyManager& km,
                                                   const std::string& pattern,
                                                   bool includeDeleted,
                                                   std::optional<uint64_t> asOfTime) {
    std::vector<std::string> matches;
    auto allKeys = km.getAllKeys(true);

    for (const auto& keyName : allKeys) {
        if (!pattern.empty() && keyName.find(pattern) == std::string::npos) {
            continue;
        }

        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        if (asOfTime.has_value()) {
            QueryResult q = TemporalSearch::queryByTimestamp(*rec, asOfTime.value());
            if (q.status == QueryStatus::FOUND || (includeDeleted && q.status == QueryStatus::DELETED)) {
                matches.push_back(keyName);
            }
        } else {
            if (includeDeleted || !rec->isDeleted) {
                matches.push_back(keyName);
            }
        }
    }

    return matches;
}

Result<void> SnapshotEngine::createNamedSnapshot(const std::string& name,
                                                uint64_t timestamp,
                                                uint64_t globalVersionId,
                                                std::string author,
                                                std::string desc) {
    auto valRes = Validation::validateSnapshotName(name);
    if (valRes.isError()) return valRes;

    if (m_namedSnapshots.find(name) != m_namedSnapshots.end()) {
        return Result<void>::error(ErrorCode::ERR_SNAPSHOT_ALREADY_EXISTS,
                                   "Snapshot '" + name + "' already exists");
    }

    NamedSnapshotMarker marker;
    marker.name = name;
    marker.timestamp = timestamp;
    marker.globalVersionId = globalVersionId;
    marker.author = std::move(author);
    marker.description = std::move(desc);

    m_namedSnapshots[name] = std::move(marker);
    return Result<void>::ok();
}

Result<void> SnapshotEngine::dropNamedSnapshot(const std::string& name) {
    auto it = m_namedSnapshots.find(name);
    if (it == m_namedSnapshots.end()) {
        return Result<void>::error(ErrorCode::ERR_SNAPSHOT_NOT_FOUND, "Snapshot '" + name + "' not found");
    }
    m_namedSnapshots.erase(it);
    return Result<void>::ok();
}

std::optional<NamedSnapshotMarker> SnapshotEngine::getNamedSnapshot(const std::string& name) const {
    auto it = m_namedSnapshots.find(name);
    if (it == m_namedSnapshots.end()) return std::nullopt;
    return it->second;
}

std::vector<NamedSnapshotMarker> SnapshotEngine::listNamedSnapshots() const {
    std::vector<NamedSnapshotMarker> list;
    list.reserve(m_namedSnapshots.size());
    for (const auto& [_, marker] : m_namedSnapshots) {
        list.push_back(marker);
    }
    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return a.name < b.name;
    });
    return list;
}

bool SnapshotEngine::hasNamedSnapshot(const std::string& name) const {
    return m_namedSnapshots.find(name) != m_namedSnapshots.end();
}

Result<VersionRecord> SnapshotEngine::rollbackKeyToVersion(KeyManager& km,
                                                           const std::string& key,
                                                           uint32_t targetVersion,
                                                           uint64_t currentTimestamp,
                                                           std::string author) {
    const KeyRecord* rec = km.getKeyRecord(key);
    if (!rec) {
        return Result<VersionRecord>::error(ErrorCode::ERR_KEY_UNKNOWN, "Cannot rollback non-existent key");
    }

    QueryResult target = TemporalSearch::queryByVersionNumber(*rec, targetVersion);
    if (target.status == QueryStatus::INVALID_VERSION) {
        return Result<VersionRecord>::error(ErrorCode::ERR_INVALID_VERSION, "Invalid target version for rollback");
    }
    if (target.status == QueryStatus::PRUNED) {
        return Result<VersionRecord>::error(ErrorCode::ERR_PRUNED, "Target version has been pruned");
    }

    if (target.status == QueryStatus::DELETED) {
        return km.prepareDelete(key, currentTimestamp, std::move(author),
                                "Rollback to deleted state at version " + std::to_string(targetVersion));
    }

    return km.prepareRollback(key, target.value, currentTimestamp, std::move(author),
                              "Rollback to version " + std::to_string(targetVersion));
}

Result<VersionRecord> SnapshotEngine::rollbackKeyToTime(KeyManager& km,
                                                        const std::string& key,
                                                        uint64_t targetTime,
                                                        uint64_t currentTimestamp,
                                                        std::string author) {
    const KeyRecord* rec = km.getKeyRecord(key);
    if (!rec) {
        return Result<VersionRecord>::error(ErrorCode::ERR_KEY_UNKNOWN, "Cannot rollback non-existent key");
    }

    QueryResult target = TemporalSearch::queryByTimestamp(*rec, targetTime);
    if (target.status == QueryStatus::NOT_YET_EXISTS) {
        return Result<VersionRecord>::error(ErrorCode::ERR_NOT_YET_EXISTS,
                                            "Key did not exist at specified target time");
    }
    if (target.status == QueryStatus::DELETED) {
        return km.prepareDelete(key, currentTimestamp, std::move(author),
                                "Rollback to deleted state at time " + Timestamp::formatIso8601(targetTime));
    }

    return km.prepareRollback(key, target.value, currentTimestamp, std::move(author),
                              "Rollback to state at time " + Timestamp::formatIso8601(targetTime));
}

Result<std::vector<VersionRecord>> SnapshotEngine::restoreNamedSnapshot(KeyManager& km,
                                                                        const std::string& name,
                                                                        uint64_t currentTimestamp,
                                                                        std::string author) {
    auto markerOpt = getNamedSnapshot(name);
    if (!markerOpt.has_value()) {
        return Result<std::vector<VersionRecord>>::error(ErrorCode::ERR_SNAPSHOT_NOT_FOUND,
                                                         "Snapshot '" + name + "' not found");
    }

    const auto& marker = markerOpt.value();
    TimeSnapshot targetSnap = takeTimeSnapshot(km, marker.timestamp);

    std::unordered_map<std::string, std::string> snapMap;
    for (const auto& entry : targetSnap.activeEntries) {
        snapMap[entry.key] = entry.value;
    }

    std::vector<VersionRecord> forwardVersions;
    auto allKeys = km.getAllKeys(true);
    uint64_t currentGlobalId = km.nextGlobalVersionId();

    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        QueryResult cur = TemporalSearch::queryCurrent(*rec);
        auto snapIt = snapMap.find(keyName);

        if (snapIt != snapMap.end()) {
            // Key existed in snapshot with value snapIt->second
            if (cur.status != QueryStatus::FOUND || cur.value != snapIt->second) {
                uint64_t gId = currentGlobalId++;
                uint32_t kVer = rec->latestVersionNumber + 1;
                uint64_t prevVer = rec->latestGlobalVersionId;
                forwardVersions.emplace_back(
                    gId, kVer, rec->keyId, keyName, snapIt->second, OpType::OP_ROLLBACK,
                    currentTimestamp, prevVer, false, 0, author, "Restore from snapshot '" + name + "'"
                );
            }
        } else {
            // Key was not active in snapshot -> if active currently, tombstone it
            if (cur.status == QueryStatus::FOUND) {
                uint64_t gId = currentGlobalId++;
                uint32_t kVer = rec->latestVersionNumber + 1;
                uint64_t prevVer = rec->latestGlobalVersionId;
                forwardVersions.emplace_back(
                    gId, kVer, rec->keyId, keyName, "", OpType::OP_DELETE,
                    currentTimestamp, prevVer, true, 0, author, "Delete to match snapshot '" + name + "'"
                );
            }
        }
    }

    return Result<std::vector<VersionRecord>>::ok(forwardVersions);
}

std::vector<VersionRecord> SnapshotEngine::sweepExpiredKeys(KeyManager& km, uint64_t currentTimestamp) {
    std::vector<VersionRecord> expiredRecords;
    auto allKeys = km.getAllKeys(false); // Active keys only
    uint64_t currentGlobalId = km.nextGlobalVersionId();

    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        if (rec->expiryTimestamp > 0 && currentTimestamp >= rec->expiryTimestamp) {
            uint64_t globalId = currentGlobalId++;
            uint32_t keyVer = rec->latestVersionNumber + 1;
            uint64_t prevVer = rec->latestGlobalVersionId;

            // Generate OP_EXPIRE tombstone record
            expiredRecords.emplace_back(globalId, keyVer, rec->keyId, keyName, "", OpType::OP_EXPIRE,
                                        currentTimestamp, prevVer, true, 0, "system", "TTL Expired");
        }
    }

    return expiredRecords;
}

void SnapshotEngine::clear() {
    m_namedSnapshots.clear();
}

} // namespace ttkv
