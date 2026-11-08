#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <utility>

namespace ttkv {

// Immutable historical record representing a single version of a key-value pair
class VersionRecord {
public:
    VersionRecord(uint64_t globalVersionId,
                  uint32_t keyVersionNumber,
                  uint32_t keyId,
                  std::string key,
                  std::string value,
                  OpType opType,
                  uint64_t timestamp,
                  uint64_t prevVersionId,
                  bool isDeleted = false,
                  uint64_t expiryTimestamp = 0,
                  std::string author = "system",
                  std::string description = "")
        : m_globalVersionId(globalVersionId)
        , m_keyVersionNumber(keyVersionNumber)
        , m_keyId(keyId)
        , m_key(std::move(key))
        , m_value(std::move(value))
        , m_opType(opType)
        , m_timestamp(timestamp)
        , m_prevVersionId(prevVersionId)
        , m_isDeleted(isDeleted)
        , m_expiryTimestamp(expiryTimestamp)
        , m_author(std::move(author))
        , m_description(std::move(description)) {}

    // Getters for immutable fields
    uint64_t globalVersionId() const { return m_globalVersionId; }
    uint32_t keyVersionNumber() const { return m_keyVersionNumber; }
    uint32_t keyId() const { return m_keyId; }
    const std::string& key() const { return m_key; }
    const std::string& value() const { return m_value; }
    OpType opType() const { return m_opType; }
    uint64_t timestamp() const { return m_timestamp; }
    uint64_t prevVersionId() const { return m_prevVersionId; }
    bool isDeleted() const { return m_isDeleted; }
    uint64_t expiryTimestamp() const { return m_expiryTimestamp; }
    const std::string& author() const { return m_author; }
    const std::string& description() const { return m_description; }

    bool isExpiredAt(uint64_t time) const {
        return (m_expiryTimestamp > 0 && time >= m_expiryTimestamp);
    }

private:
    uint64_t m_globalVersionId;
    uint32_t m_keyVersionNumber;
    uint32_t m_keyId;
    std::string m_key;
    std::string m_value;
    OpType m_opType;
    uint64_t m_timestamp;
    uint64_t m_prevVersionId;
    bool m_isDeleted;
    uint64_t m_expiryTimestamp;
    std::string m_author;
    std::string m_description;
};

// Represents an active key entry in the store with reference to its complete history
struct KeyRecord {
    uint32_t keyId{0};
    std::string key;
    uint32_t latestVersionNumber{0};
    uint64_t latestGlobalVersionId{0};
    bool isDeleted{false};
    uint64_t expiryTimestamp{0};
    std::vector<VersionRecord> versions; // Sequential immutable version history
};

// Result structure returned by time-travel queries
struct QueryResult {
    QueryStatus status{QueryStatus::KEY_UNKNOWN};
    std::string key;
    std::string value;
    uint32_t versionNumber{0};
    uint64_t globalVersionId{0};
    uint64_t timestamp{0};
    bool isDeleted{false};
    uint64_t expiryTimestamp{0};
    std::string author;
    std::string description;

    static QueryResult found(const VersionRecord& v) {
        return QueryResult{
            QueryStatus::FOUND,
            v.key(),
            v.value(),
            v.keyVersionNumber(),
            v.globalVersionId(),
            v.timestamp(),
            v.isDeleted(),
            v.expiryTimestamp(),
            v.author(),
            v.description()
        };
    }

    static QueryResult deleted(const VersionRecord& v) {
        return QueryResult{
            QueryStatus::DELETED,
            v.key(),
            "",
            v.keyVersionNumber(),
            v.globalVersionId(),
            v.timestamp(),
            true,
            v.expiryTimestamp(),
            v.author(),
            v.description()
        };
    }

    static QueryResult expired(const VersionRecord& v) {
        return QueryResult{
            QueryStatus::EXPIRED,
            v.key(),
            "",
            v.keyVersionNumber(),
            v.globalVersionId(),
            v.timestamp(),
            v.isDeleted(),
            v.expiryTimestamp(),
            v.author(),
            v.description()
        };
    }

    static QueryResult statusOnly(QueryStatus st, std::string keyName = "") {
        QueryResult r;
        r.status = st;
        r.key = std::move(keyName);
        return r;
    }
};

} // namespace ttkv
