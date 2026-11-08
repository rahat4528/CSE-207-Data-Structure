#pragma once

#include "Common.h"
#include "DataModel.h"
#include "Validation.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <optional>

namespace ttkv {

class KeyManager {
public:
    KeyManager();

    // Key existence and state queries
    bool hasKey(const std::string& key) const;
    bool isDeleted(const std::string& key) const;
    const KeyRecord* getKeyRecord(const std::string& key) const;

    // Prepares new immutable VersionRecords for operations
    Result<VersionRecord> prepareWrite(const std::string& key,
                                       const std::string& value,
                                       uint64_t timestamp,
                                       uint64_t expiry = 0,
                                       std::string author = "system",
                                       std::string desc = "");

    Result<VersionRecord> prepareDelete(const std::string& key,
                                        uint64_t timestamp,
                                        std::string author = "system",
                                        std::string desc = "");

    Result<VersionRecord> prepareRollback(const std::string& key,
                                          const std::string& restoredValue,
                                          uint64_t timestamp,
                                          std::string author = "system",
                                          std::string desc = "");

    // Applies a committed VersionRecord to in-memory state (used by writes and WAL replay)
    // Enforces HashSet duplicate-ID rejection and version chain linkage
    Result<void> applyRecord(const VersionRecord& record);

    // Queries
    std::vector<std::string> getAllKeys(bool includeDeleted = true) const;
    size_t keyCount(bool includeDeleted = true) const;
    size_t totalVersionCount() const;

    uint64_t nextGlobalVersionId() const { return m_nextGlobalVersionId; }
    void setNextGlobalVersionId(uint64_t id) { m_nextGlobalVersionId = id; }

    uint32_t nextKeyId() const { return m_nextKeyId; }
    void setNextKeyId(uint32_t id) { m_nextKeyId = id; }

    void clear();

private:
    std::unordered_map<std::string, KeyRecord> m_keys;      // HashMap: Key name -> KeyRecord
    std::unordered_set<uint64_t> m_versionIds;               // HashSet: Duplicate version ID detection
    uint64_t m_nextGlobalVersionId{1};
    uint32_t m_nextKeyId{1};
    size_t m_totalVersions{0};
};

} // namespace ttkv
