#pragma once

#include "Common.h"
#include "DataModel.h"
#include "KeyManager.h"
#include "TemporalSearch.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cstdint>

namespace ttkv {

struct KeyDiff {
    std::string key;
    QueryResult stateA;
    QueryResult stateB;
    std::string changeType; // "ADDED", "MODIFIED", "DELETED", "UNCHANGED", "NOT_EXISTENT"
};

struct StoreDiff {
    uint64_t timeA{0};
    uint64_t timeB{0};
    std::vector<KeyDiff> added;
    std::vector<KeyDiff> modified;
    std::vector<KeyDiff> deleted;
    std::vector<KeyDiff> unchanged;
};

struct TimeSnapshot {
    uint64_t timestamp{0};
    std::string formattedTime;
    std::vector<QueryResult> activeEntries;
};

struct NamedSnapshotMarker {
    std::string name;
    uint64_t timestamp{0};
    uint64_t globalVersionId{0};
    std::string author;
    std::string description;
};

class SnapshotEngine {
public:
    SnapshotEngine();

    // Compares a single key between two timestamps (O(log V))
    static Result<KeyDiff> diffKey(const KeyRecord& keyRec, uint64_t timeA, uint64_t timeB);

    // Compares the entire store between two timestamps (O(K log V))
    static Result<StoreDiff> diffStore(const KeyManager& km, uint64_t timeA, uint64_t timeB);

    // Resolves state of all active keys as of a specific point in time (O(K log V))
    static TimeSnapshot takeTimeSnapshot(const KeyManager& km, uint64_t targetTime);

    // Substring / prefix search across keys (O(K)) with optional historical filtering
    static std::vector<std::string> searchKeys(const KeyManager& km,
                                              const std::string& pattern,
                                              bool includeDeleted = false,
                                              std::optional<uint64_t> asOfTime = std::nullopt);

    // Named Snapshot Management (Lightweight O(1) markers protecting versions from pruning)
    Result<void> createNamedSnapshot(const std::string& name,
                                    uint64_t timestamp,
                                    uint64_t globalVersionId,
                                    std::string author = "system",
                                    std::string desc = "");

    Result<void> dropNamedSnapshot(const std::string& name);
    std::optional<NamedSnapshotMarker> getNamedSnapshot(const std::string& name) const;
    std::vector<NamedSnapshotMarker> listNamedSnapshots() const;
    bool hasNamedSnapshot(const std::string& name) const;

    // Rollback key by creating a NEW version restoring prior state (never erases history)
    static Result<VersionRecord> rollbackKeyToVersion(KeyManager& km,
                                                      const std::string& key,
                                                      uint32_t targetVersion,
                                                      uint64_t currentTimestamp,
                                                      std::string author = "system");

    static Result<VersionRecord> rollbackKeyToTime(KeyManager& km,
                                                   const std::string& key,
                                                   uint64_t targetTime,
                                                   uint64_t currentTimestamp,
                                                   std::string author = "system");

    // Restores named snapshot state by creating new forward versions for all differing keys
    Result<std::vector<VersionRecord>> restoreNamedSnapshot(KeyManager& km,
                                                           const std::string& name,
                                                           uint64_t currentTimestamp,
                                                           std::string author = "system");

    // Sweeps expired keys, generating OP_EXPIRE tombstone versions
    static std::vector<VersionRecord> sweepExpiredKeys(KeyManager& km, uint64_t currentTimestamp);

    void clear();

private:
    std::unordered_map<std::string, NamedSnapshotMarker> m_namedSnapshots;
};

} // namespace ttkv
