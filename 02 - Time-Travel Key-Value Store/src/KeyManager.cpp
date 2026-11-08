#include "KeyManager.h"
#include <algorithm>

namespace ttkv {

KeyManager::KeyManager() = default;

bool KeyManager::hasKey(const std::string& key) const {
    return m_keys.find(key) != m_keys.end();
}

bool KeyManager::isDeleted(const std::string& key) const {
    auto it = m_keys.find(key);
    if (it == m_keys.end()) return false;
    return it->second.isDeleted;
}

const KeyRecord* KeyManager::getKeyRecord(const std::string& key) const {
    auto it = m_keys.find(key);
    if (it == m_keys.end()) return nullptr;
    return &it->second;
}

Result<VersionRecord> KeyManager::prepareWrite(const std::string& key,
                                               const std::string& value,
                                               uint64_t timestamp,
                                               uint64_t expiry,
                                               std::string author,
                                               std::string desc) {
    auto keyVal = Validation::validateKey(key);
    if (keyVal.isError()) return Result<VersionRecord>::error(keyVal.error(), keyVal.message());

    auto valVal = Validation::validateValue(value);
    if (valVal.isError()) return Result<VersionRecord>::error(valVal.error(), valVal.message());

    uint64_t globalId = m_nextGlobalVersionId;
    auto it = m_keys.find(key);
    if (it == m_keys.end()) {
        // Initial creation (OP_SET)
        uint32_t keyId = m_nextKeyId;
        uint32_t keyVer = 1;
        uint64_t prevVer = 0;
        return Result<VersionRecord>::ok(VersionRecord(
            globalId, keyVer, keyId, key, value, OpType::OP_SET,
            timestamp, prevVer, false, expiry, std::move(author), std::move(desc)
        ));
    } else {
        // Update to existing key (OP_UPDATE) - even after previous deletion
        const auto& rec = it->second;
        uint32_t keyId = rec.keyId;
        uint32_t keyVer = rec.latestVersionNumber + 1;
        uint64_t prevVer = rec.latestGlobalVersionId;
        return Result<VersionRecord>::ok(VersionRecord(
            globalId, keyVer, keyId, key, value, OpType::OP_UPDATE,
            timestamp, prevVer, false, expiry, std::move(author), std::move(desc)
        ));
    }
}

Result<VersionRecord> KeyManager::prepareDelete(const std::string& key,
                                                uint64_t timestamp,
                                                std::string author,
                                                std::string desc) {
    auto keyVal = Validation::validateKey(key);
    if (keyVal.isError()) return Result<VersionRecord>::error(keyVal.error(), keyVal.message());

    auto it = m_keys.find(key);
    if (it == m_keys.end()) {
        return Result<VersionRecord>::error(ErrorCode::ERR_KEY_UNKNOWN, "Cannot delete non-existent key");
    }
    if (it->second.isDeleted) {
        return Result<VersionRecord>::error(ErrorCode::ERR_KEY_DELETED, "Key is already deleted");
    }

    const auto& rec = it->second;
    uint64_t globalId = m_nextGlobalVersionId;
    uint32_t keyId = rec.keyId;
    uint32_t keyVer = rec.latestVersionNumber + 1;
    uint64_t prevVer = rec.latestGlobalVersionId;

    // Tombstone record: isDeleted = true, value = ""
    return Result<VersionRecord>::ok(VersionRecord(
        globalId, keyVer, keyId, key, "", OpType::OP_DELETE,
        timestamp, prevVer, true, 0, std::move(author), std::move(desc)
    ));
}

Result<VersionRecord> KeyManager::prepareRollback(const std::string& key,
                                                  const std::string& restoredValue,
                                                  uint64_t timestamp,
                                                  std::string author,
                                                  std::string desc) {
    auto keyVal = Validation::validateKey(key);
    if (keyVal.isError()) return Result<VersionRecord>::error(keyVal.error(), keyVal.message());

    auto valVal = Validation::validateValue(restoredValue);
    if (valVal.isError()) return Result<VersionRecord>::error(valVal.error(), valVal.message());

    auto it = m_keys.find(key);
    if (it == m_keys.end()) {
        return Result<VersionRecord>::error(ErrorCode::ERR_KEY_UNKNOWN, "Cannot rollback non-existent key");
    }

    const auto& rec = it->second;
    uint64_t globalId = m_nextGlobalVersionId;
    uint32_t keyId = rec.keyId;
    uint32_t keyVer = rec.latestVersionNumber + 1;
    uint64_t prevVer = rec.latestGlobalVersionId;

    return Result<VersionRecord>::ok(VersionRecord(
        globalId, keyVer, keyId, key, restoredValue, OpType::OP_ROLLBACK,
        timestamp, prevVer, false, 0, std::move(author), std::move(desc)
    ));
}

Result<void> KeyManager::applyRecord(const VersionRecord& record) {
    // 1. Check for duplicate global version ID using HashSet
    if (m_versionIds.find(record.globalVersionId()) != m_versionIds.end()) {
        return Result<void>::error(ErrorCode::ERR_DUPLICATE_VERSION_ID,
                                   "Duplicate global version ID detected: " +
                                   std::to_string(record.globalVersionId()));
    }

    // 2. Find or create key record
    auto it = m_keys.find(record.key());
    if (it == m_keys.end()) {
        // Initial version for key
        if (record.prevVersionId() != 0) {
            return Result<void>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                       "Initial version has non-zero prevVersionId");
        }
        KeyRecord rec;
        rec.keyId = record.keyId();
        rec.key = record.key();
        rec.latestVersionNumber = record.keyVersionNumber();
        rec.latestGlobalVersionId = record.globalVersionId();
        rec.isDeleted = record.isDeleted();
        rec.expiryTimestamp = record.expiryTimestamp();
        rec.versions.push_back(record);

        m_keys[record.key()] = std::move(rec);
        if (record.keyId() >= m_nextKeyId) {
            m_nextKeyId = record.keyId() + 1;
        }
    } else {
        // Subsequent version for existing key
        auto& rec = it->second;
        if (record.prevVersionId() != rec.latestGlobalVersionId) {
            return Result<void>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                       "Version chain broken: expected prevVersionId " +
                                       std::to_string(rec.latestGlobalVersionId) + " but got " +
                                       std::to_string(record.prevVersionId()));
        }
        if (record.keyVersionNumber() <= rec.latestVersionNumber) {
            return Result<void>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                       "Per-key version number must strictly increase");
        }

        rec.latestVersionNumber = record.keyVersionNumber();
        rec.latestGlobalVersionId = record.globalVersionId();
        rec.isDeleted = record.isDeleted();
        rec.expiryTimestamp = record.expiryTimestamp();
        rec.versions.push_back(record);
    }

    // 3. Register global version ID and increment counters
    m_versionIds.insert(record.globalVersionId());
    m_totalVersions++;
    if (record.globalVersionId() >= m_nextGlobalVersionId) {
        m_nextGlobalVersionId = record.globalVersionId() + 1;
    }

    return Result<void>::ok();
}

std::vector<std::string> KeyManager::getAllKeys(bool includeDeleted) const {
    std::vector<std::string> result;
    result.reserve(m_keys.size());
    for (const auto& [name, rec] : m_keys) {
        if (includeDeleted || !rec.isDeleted) {
            result.push_back(name);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

size_t KeyManager::keyCount(bool includeDeleted) const {
    if (includeDeleted) return m_keys.size();
    size_t count = 0;
    for (const auto& [_, rec] : m_keys) {
        if (!rec.isDeleted) count++;
    }
    return count;
}

size_t KeyManager::totalVersionCount() const {
    return m_totalVersions;
}

void KeyManager::clear() {
    m_keys.clear();
    m_versionIds.clear();
    m_nextGlobalVersionId = 1;
    m_nextKeyId = 1;
    m_totalVersions = 0;
}

} // namespace ttkv
