#pragma once

#include "Common.h"
#include "DataModel.h"
#include "IClock.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "AuditLogger.h"
#include "SnapshotEngine.h"
#include "TransactionManager.h"
#include "StorageEngine.h"
#include "HistoryEngine.h"
#include <string>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <optional>

namespace ttkv {

class TimeTravelStore {
public:
    explicit TimeTravelStore(std::string walPath = "",
                             DurabilityLevel durability = DurabilityLevel::LEVEL1_FLUSH,
                             std::unique_ptr<IClock> clock = std::make_unique<SystemClock>());
    ~TimeTravelStore();

    // Initialization & Recovery
    Result<size_t> initialize(bool recoveryMode = false);
    void close();

    // Core Key-Value Operations (Thread-safe)
    Result<uint64_t> set(const std::string& key,
                         const std::string& value,
                         uint64_t ttlMicros = 0,
                         std::string author = "user",
                         std::string desc = "");

    Result<uint64_t> del(const std::string& key,
                         std::string author = "user",
                         std::string desc = "");

    // Time-Travel Read Operations (Shared Lock / Concurrent Readers)
    Result<QueryResult> get(const std::string& key) const;
    Result<QueryResult> getAtVersion(const std::string& key, uint32_t version) const;
    Result<QueryResult> getAtTime(const std::string& key, uint64_t timestampMicros) const;
    Result<QueryResult> getAtTimeIso(const std::string& key, const std::string& isoString) const;

    // History and Comparison Queries
    Result<std::vector<HistoryEntry>> history(const std::string& key,
                                             size_t limit = 0,
                                             size_t offset = 0) const;

    Result<KeyDiff> diff(const std::string& key, uint64_t timeA, uint64_t timeB) const;
    Result<StoreDiff> diffStore(uint64_t timeA, uint64_t timeB) const;
    Result<TimeSnapshot> snapshotAtTime(uint64_t timestamp) const;

    // User-Level Rollback (creates a new version restoring past value)
    Result<uint64_t> rollbackToVersion(const std::string& key, uint32_t version, std::string author = "user");
    Result<uint64_t> rollbackToTime(const std::string& key, uint64_t time, std::string author = "user");

    // Named Snapshot Management
    Result<void> createNamedSnapshot(const std::string& name, std::string author = "user", std::string desc = "");
    Result<void> dropNamedSnapshot(const std::string& name);
    Result<void> restoreNamedSnapshot(const std::string& name, std::string author = "user");
    std::vector<NamedSnapshotMarker> listNamedSnapshots() const;

    // Search and Expiration
    std::vector<std::string> searchKeys(const std::string& pattern, bool includeDeleted = false) const;
    size_t sweepExpired();

    // Atomic Transactions
    Result<uint64_t> beginTransaction(std::string author = "user");
    Result<size_t> commitTransaction();
    Result<void> abortTransaction();
    bool isInTransaction() const;

    // Subsystem Accessors
    const KeyManager& keyManager() const { return m_keyManager; }
    const EventLog& eventLog() const { return m_eventLog; }
    const AuditLogger& auditLogger() const { return m_auditLogger; }
    AuditLogger& auditLogger() { return m_auditLogger; }
    IClock& clock() { return *m_clock; }

private:
    mutable std::shared_mutex m_rwLock;
    KeyManager m_keyManager;
    EventLog m_eventLog;
    AuditLogger m_auditLogger;
    SnapshotEngine m_snapshotEngine;
    TransactionManager m_txManager;
    std::unique_ptr<StorageEngine> m_storage;
    std::unique_ptr<IClock> m_clock;
    std::string m_walPath;
    DurabilityLevel m_durability;
};

} // namespace ttkv
