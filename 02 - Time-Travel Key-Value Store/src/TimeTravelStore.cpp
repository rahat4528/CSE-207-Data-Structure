#include "TimeTravelStore.h"
#include "Timestamp.h"

namespace ttkv {

TimeTravelStore::TimeTravelStore(std::string walPath,
                                 DurabilityLevel durability,
                                 std::unique_ptr<IClock> clock)
    : m_clock(std::move(clock))
    , m_walPath(std::move(walPath))
    , m_durability(durability) {
    if (!m_walPath.empty()) {
        m_storage = std::make_unique<StorageEngine>(m_walPath, m_durability);
    }
}

TimeTravelStore::~TimeTravelStore() {
    close();
}

Result<size_t> TimeTravelStore::initialize(bool recoveryMode) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    if (!m_storage) return Result<size_t>::ok(0);

    auto openRes = m_storage->open();
    if (openRes.isError()) return Result<size_t>::error(openRes.error(), openRes.message());

    return m_storage->recover(m_keyManager, m_eventLog, recoveryMode);
}

void TimeTravelStore::close() {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    if (m_storage) {
        m_storage->close();
    }
}

Result<uint64_t> TimeTravelStore::set(const std::string& key,
                                      const std::string& value,
                                      uint64_t ttlMicros,
                                      std::string author,
                                      std::string desc) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();
    uint64_t exp = (ttlMicros > 0) ? (now + ttlMicros) : 0;

    if (m_txManager.isInTransaction()) {
        auto stageRes = m_txManager.stageWrite(key, value, exp, desc);
        if (stageRes.isError()) return Result<uint64_t>::error(stageRes.error(), stageRes.message());
        m_auditLogger.log(now, "STAGE_SET", key, author, true, ErrorCode::OK, desc);
        return Result<uint64_t>::ok(m_txManager.currentTxId());
    }

    auto prepRes = m_keyManager.prepareWrite(key, value, now, exp, author, desc);
    if (prepRes.isError()) {
        m_auditLogger.log(now, "SET", key, author, false, prepRes.error(), prepRes.message());
        return Result<uint64_t>::error(prepRes.error(), prepRes.message());
    }

    const auto& rec = prepRes.value();
    auto applyRes = m_keyManager.applyRecord(rec);
    if (applyRes.isError()) {
        m_auditLogger.log(now, "SET", key, author, false, applyRes.error(), applyRes.message());
        return Result<uint64_t>::error(applyRes.error(), applyRes.message());
    }

    m_eventLog.appendEvent(rec);
    if (m_storage) {
        m_storage->appendRecord(rec);
    }

    m_auditLogger.log(now, "SET", key, author, true, ErrorCode::OK, desc);
    return Result<uint64_t>::ok(rec.globalVersionId());
}

Result<uint64_t> TimeTravelStore::del(const std::string& key,
                                      std::string author,
                                      std::string desc) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();

    if (m_txManager.isInTransaction()) {
        auto stageRes = m_txManager.stageDelete(key, desc);
        if (stageRes.isError()) return Result<uint64_t>::error(stageRes.error(), stageRes.message());
        m_auditLogger.log(now, "STAGE_DELETE", key, author, true, ErrorCode::OK, desc);
        return Result<uint64_t>::ok(m_txManager.currentTxId());
    }

    auto prepRes = m_keyManager.prepareDelete(key, now, author, desc);
    if (prepRes.isError()) {
        m_auditLogger.log(now, "DELETE", key, author, false, prepRes.error(), prepRes.message());
        return Result<uint64_t>::error(prepRes.error(), prepRes.message());
    }

    const auto& rec = prepRes.value();
    auto applyRes = m_keyManager.applyRecord(rec);
    if (applyRes.isError()) {
        m_auditLogger.log(now, "DELETE", key, author, false, applyRes.error(), applyRes.message());
        return Result<uint64_t>::error(applyRes.error(), applyRes.message());
    }

    m_eventLog.appendEvent(rec);
    if (m_storage) {
        m_storage->appendRecord(rec);
    }

    m_auditLogger.log(now, "DELETE", key, author, true, ErrorCode::OK, desc);
    return Result<uint64_t>::ok(rec.globalVersionId());
}

Result<QueryResult> TimeTravelStore::get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);

    // Read staged first if inside transaction
    if (m_txManager.isInTransaction()) {
        auto staged = m_txManager.readStaged(key);
        if (staged.has_value()) {
            return Result<QueryResult>::ok(staged.value());
        }
    }

    const KeyRecord* rec = m_keyManager.getKeyRecord(key);
    if (!rec) {
        return Result<QueryResult>::ok(QueryResult::statusOnly(QueryStatus::KEY_UNKNOWN, key));
    }

    QueryResult res = TemporalSearch::queryCurrent(*rec);
    return Result<QueryResult>::ok(res);
}

Result<QueryResult> TimeTravelStore::getAtVersion(const std::string& key, uint32_t version) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    const KeyRecord* rec = m_keyManager.getKeyRecord(key);
    if (!rec) {
        return Result<QueryResult>::ok(QueryResult::statusOnly(QueryStatus::KEY_UNKNOWN, key));
    }

    QueryResult res = TemporalSearch::queryByVersionNumber(*rec, version);
    return Result<QueryResult>::ok(res);
}

Result<QueryResult> TimeTravelStore::getAtTime(const std::string& key, uint64_t timestampMicros) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    const KeyRecord* rec = m_keyManager.getKeyRecord(key);
    if (!rec) {
        return Result<QueryResult>::ok(QueryResult::statusOnly(QueryStatus::KEY_UNKNOWN, key));
    }

    QueryResult res = TemporalSearch::queryByTimestamp(*rec, timestampMicros);
    return Result<QueryResult>::ok(res);
}

Result<QueryResult> TimeTravelStore::getAtTimeIso(const std::string& key, const std::string& isoString) const {
    auto parsed = Timestamp::parseIso8601(isoString);
    if (parsed.isError()) {
        return Result<QueryResult>::error(parsed.error(), parsed.message());
    }
    return getAtTime(key, parsed.value());
}

Result<std::vector<HistoryEntry>> TimeTravelStore::history(const std::string& key,
                                                          size_t limit,
                                                          size_t offset) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    const KeyRecord* rec = m_keyManager.getKeyRecord(key);
    if (!rec) {
        return Result<std::vector<HistoryEntry>>::error(ErrorCode::ERR_KEY_UNKNOWN, "Key does not exist");
    }
    return HistoryEngine::getKeyHistory(*rec, limit, offset);
}

Result<KeyDiff> TimeTravelStore::diff(const std::string& key, uint64_t timeA, uint64_t timeB) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    const KeyRecord* rec = m_keyManager.getKeyRecord(key);
    if (!rec) {
        return Result<KeyDiff>::error(ErrorCode::ERR_KEY_UNKNOWN, "Key does not exist");
    }
    return SnapshotEngine::diffKey(*rec, timeA, timeB);
}

Result<StoreDiff> TimeTravelStore::diffStore(uint64_t timeA, uint64_t timeB) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    return SnapshotEngine::diffStore(m_keyManager, timeA, timeB);
}

Result<TimeSnapshot> TimeTravelStore::snapshotAtTime(uint64_t timestamp) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    return Result<TimeSnapshot>::ok(SnapshotEngine::takeTimeSnapshot(m_keyManager, timestamp));
}

Result<uint64_t> TimeTravelStore::rollbackToVersion(const std::string& key, uint32_t version, std::string author) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();

    auto rbRes = SnapshotEngine::rollbackKeyToVersion(m_keyManager, key, version, now, author);
    if (rbRes.isError()) return Result<uint64_t>::error(rbRes.error(), rbRes.message());

    const auto& rec = rbRes.value();
    m_keyManager.applyRecord(rec);
    m_eventLog.appendEvent(rec);
    if (m_storage) {
        m_storage->appendRecord(rec);
    }

    m_auditLogger.log(now, "ROLLBACK", key, author, true, ErrorCode::OK, "Rollback to version " + std::to_string(version));
    return Result<uint64_t>::ok(rec.globalVersionId());
}

Result<uint64_t> TimeTravelStore::rollbackToTime(const std::string& key, uint64_t time, std::string author) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();

    auto rbRes = SnapshotEngine::rollbackKeyToTime(m_keyManager, key, time, now, author);
    if (rbRes.isError()) return Result<uint64_t>::error(rbRes.error(), rbRes.message());

    const auto& rec = rbRes.value();
    m_keyManager.applyRecord(rec);
    m_eventLog.appendEvent(rec);
    if (m_storage) {
        m_storage->appendRecord(rec);
    }

    m_auditLogger.log(now, "ROLLBACK", key, author, true, ErrorCode::OK, "Rollback to time " + Timestamp::formatIso8601(time));
    return Result<uint64_t>::ok(rec.globalVersionId());
}

Result<void> TimeTravelStore::createNamedSnapshot(const std::string& name, std::string author, std::string desc) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();
    uint64_t gId = m_keyManager.nextGlobalVersionId() - 1;

    auto res = m_snapshotEngine.createNamedSnapshot(name, now, gId, author, desc);
    if (res.isOk()) {
        m_auditLogger.log(now, "SNAPSHOT_CREATE", name, author, true, ErrorCode::OK, desc);
    }
    return res;
}

Result<void> TimeTravelStore::dropNamedSnapshot(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();

    auto res = m_snapshotEngine.dropNamedSnapshot(name);
    if (res.isOk()) {
        m_auditLogger.log(now, "SNAPSHOT_DROP", name, "user", true);
    }
    return res;
}

Result<void> TimeTravelStore::restoreNamedSnapshot(const std::string& name, std::string author) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();

    auto restoreRes = m_snapshotEngine.restoreNamedSnapshot(m_keyManager, name, now, author);
    if (restoreRes.isError()) return Result<void>::error(restoreRes.error(), restoreRes.message());

    for (const auto& rec : restoreRes.value()) {
        m_keyManager.applyRecord(rec);
        m_eventLog.appendEvent(rec);
        if (m_storage) {
            m_storage->appendRecord(rec);
        }
    }

    m_auditLogger.log(now, "SNAPSHOT_RESTORE", name, author, true, ErrorCode::OK,
                      "Restored " + std::to_string(restoreRes.value().size()) + " records");
    return Result<void>::ok();
}

std::vector<NamedSnapshotMarker> TimeTravelStore::listNamedSnapshots() const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    return m_snapshotEngine.listNamedSnapshots();
}

std::vector<std::string> TimeTravelStore::searchKeys(const std::string& pattern, bool includeDeleted) const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    return SnapshotEngine::searchKeys(m_keyManager, pattern, includeDeleted);
}

size_t TimeTravelStore::sweepExpired() {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();

    auto swept = SnapshotEngine::sweepExpiredKeys(m_keyManager, now);
    for (const auto& rec : swept) {
        m_keyManager.applyRecord(rec);
        m_eventLog.appendEvent(rec);
        if (m_storage) {
            m_storage->appendRecord(rec);
        }
        m_auditLogger.log(now, "EXPIRE_SWEEP", rec.key(), "system", true);
    }

    return swept.size();
}

Result<uint64_t> TimeTravelStore::beginTransaction(std::string author) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();
    auto res = m_txManager.begin(now, author);
    if (res.isOk()) {
        m_auditLogger.log(now, "BEGIN", "", author, true);
    }
    return res;
}

Result<size_t> TimeTravelStore::commitTransaction() {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();
    auto res = m_txManager.commit(m_keyManager, m_eventLog, m_storage.get(), now);
    if (res.isOk()) {
        m_auditLogger.log(now, "COMMIT", "", "user", true, ErrorCode::OK,
                          "Committed " + std::to_string(res.value()) + " operations");
    }
    return res;
}

Result<void> TimeTravelStore::abortTransaction() {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    uint64_t now = m_clock->nowMicros();
    auto res = m_txManager.abort();
    if (res.isOk()) {
        m_auditLogger.log(now, "ABORT", "", "user", true);
    }
    return res;
}

bool TimeTravelStore::isInTransaction() const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    return m_txManager.isInTransaction();
}

} // namespace ttkv
