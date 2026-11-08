#include "TransactionManager.h"
#include "Validation.h"

namespace ttkv {

TransactionManager::TransactionManager() = default;

Result<uint64_t> TransactionManager::begin(uint64_t timestamp, std::string author) {
    if (m_activeContext.has_value()) {
        return Result<uint64_t>::error(ErrorCode::ERR_TRANSACTION_ACTIVE,
                                       "Cannot begin transaction: a transaction is already active");
    }

    uint64_t txId = m_nextTxId++;
    ActiveContext ctx;
    ctx.txId = txId;
    ctx.startTimestamp = timestamp;
    ctx.author = std::move(author);

    m_activeContext = std::move(ctx);
    return Result<uint64_t>::ok(txId);
}

Result<void> TransactionManager::stageWrite(const std::string& key,
                                           const std::string& value,
                                           uint64_t expiry,
                                           std::string desc) {
    if (!m_activeContext.has_value()) {
        return Result<void>::error(ErrorCode::ERR_NO_ACTIVE_TRANSACTION, "No active transaction");
    }

    auto keyVal = Validation::validateKey(key);
    if (keyVal.isError()) return keyVal;

    auto valVal = Validation::validateValue(value);
    if (valVal.isError()) return valVal;

    auto& ctx = m_activeContext.value();

    // Push undo entry to Rollback Stack
    TxUndoEntry undo;
    undo.key = key;
    undo.existedInStaged = (ctx.stagedWrites.find(key) != ctx.stagedWrites.end());
    if (undo.existedInStaged) {
        undo.previousStagedValue = ctx.stagedWrites[key];
    }
    undo.wasStagedDeleted = (ctx.stagedDeletions.find(key) != ctx.stagedDeletions.end());
    ctx.rollbackStack.push(std::move(undo));

    // Update staged overlay
    ctx.stagedDeletions.erase(key);
    ctx.stagedWrites[key] = value;

    TxOperation op;
    op.opType = OpType::OP_UPDATE;
    op.key = key;
    op.value = value;
    op.expiryTimestamp = expiry;
    op.description = std::move(desc);
    ctx.operations.push_back(std::move(op));

    return Result<void>::ok();
}

Result<void> TransactionManager::stageDelete(const std::string& key, std::string desc) {
    if (!m_activeContext.has_value()) {
        return Result<void>::error(ErrorCode::ERR_NO_ACTIVE_TRANSACTION, "No active transaction");
    }

    auto keyVal = Validation::validateKey(key);
    if (keyVal.isError()) return keyVal;

    auto& ctx = m_activeContext.value();

    // Push undo entry to Rollback Stack
    TxUndoEntry undo;
    undo.key = key;
    undo.existedInStaged = (ctx.stagedWrites.find(key) != ctx.stagedWrites.end());
    if (undo.existedInStaged) {
        undo.previousStagedValue = ctx.stagedWrites[key];
    }
    undo.wasStagedDeleted = (ctx.stagedDeletions.find(key) != ctx.stagedDeletions.end());
    ctx.rollbackStack.push(std::move(undo));

    // Update staged overlay
    ctx.stagedWrites.erase(key);
    ctx.stagedDeletions.insert(key);

    TxOperation op;
    op.opType = OpType::OP_DELETE;
    op.key = key;
    op.value = "";
    op.expiryTimestamp = 0;
    op.description = std::move(desc);
    ctx.operations.push_back(std::move(op));

    return Result<void>::ok();
}

std::optional<QueryResult> TransactionManager::readStaged(const std::string& key) const {
    if (!m_activeContext.has_value()) return std::nullopt;

    const auto& ctx = m_activeContext.value();
    if (ctx.stagedDeletions.find(key) != ctx.stagedDeletions.end()) {
        QueryResult r;
        r.status = QueryStatus::DELETED;
        r.key = key;
        r.isDeleted = true;
        return r;
    }

    auto it = ctx.stagedWrites.find(key);
    if (it != ctx.stagedWrites.end()) {
        QueryResult r;
        r.status = QueryStatus::FOUND;
        r.key = key;
        r.value = it->second;
        return r;
    }

    return std::nullopt;
}

Result<void> TransactionManager::abort() {
    if (!m_activeContext.has_value()) {
        return Result<void>::error(ErrorCode::ERR_NO_ACTIVE_TRANSACTION, "No active transaction to abort");
    }

    auto& ctx = m_activeContext.value();

    // Unwind using Rollback Stack
    while (!ctx.rollbackStack.empty()) {
        auto undo = ctx.rollbackStack.top();
        ctx.rollbackStack.pop();

        if (undo.existedInStaged) {
            ctx.stagedWrites[undo.key] = undo.previousStagedValue.value();
        } else {
            ctx.stagedWrites.erase(undo.key);
        }

        if (undo.wasStagedDeleted) {
            ctx.stagedDeletions.insert(undo.key);
        } else {
            ctx.stagedDeletions.erase(undo.key);
        }
    }

    TransactionRecord rec;
    rec.txId = ctx.txId;
    rec.startTimestamp = ctx.startTimestamp;
    rec.commitTimestamp = 0;
    rec.committed = false;
    rec.operationCount = ctx.operations.size();
    rec.author = ctx.author;
    m_history.push_back(std::move(rec));

    m_activeContext.reset();
    return Result<void>::ok();
}

Result<size_t> TransactionManager::commit(KeyManager& km,
                                         EventLog& el,
                                         StorageEngine* storage,
                                         uint64_t commitTimestamp) {
    if (!m_activeContext.has_value()) {
        return Result<size_t>::error(ErrorCode::ERR_NO_ACTIVE_TRANSACTION, "No active transaction to commit");
    }

    auto& ctx = m_activeContext.value();
    if (ctx.operations.empty()) {
        // Empty transaction: simple commit
        TransactionRecord rec;
        rec.txId = ctx.txId;
        rec.startTimestamp = ctx.startTimestamp;
        rec.commitTimestamp = commitTimestamp;
        rec.committed = true;
        rec.operationCount = 0;
        rec.author = ctx.author;
        m_history.push_back(std::move(rec));
        m_activeContext.reset();
        return Result<size_t>::ok(0);
    }

    // 1. Dry-run phase: validate and prepare all version records
    // Any failure during dry run causes total abort with zero changes committed
    std::vector<VersionRecord> preparedBatch;
    preparedBatch.reserve(ctx.operations.size());

    // Temporary copy of KeyManager counters and latest versions
    KeyManager tempKm = km;

    for (const auto& op : ctx.operations) {
        Result<VersionRecord> prepRes = Result<VersionRecord>::error(ErrorCode::ERR_SYNTAX_ERROR);
        if (op.opType == OpType::OP_DELETE) {
            prepRes = tempKm.prepareDelete(op.key, commitTimestamp, ctx.author, op.description);
        } else {
            prepRes = tempKm.prepareWrite(op.key, op.value, commitTimestamp, op.expiryTimestamp,
                                          ctx.author, op.description);
        }

        if (prepRes.isError()) {
            abort(); // Unwind staged state
            return Result<size_t>::error(prepRes.error(),
                                         "Transaction validation failed on key '" + op.key + "': " + prepRes.message());
        }

        auto applyRes = tempKm.applyRecord(prepRes.value());
        if (applyRes.isError()) {
            abort();
            return Result<size_t>::error(applyRes.error(),
                                         "Transaction dry-run failed applying key '" + op.key + "': " + applyRes.message());
        }

        preparedBatch.push_back(prepRes.value());
    }

    // 2. Commit phase: all records validated, now atomically apply to KeyManager, EventLog, and Storage
    for (const auto& rec : preparedBatch) {
        km.applyRecord(rec);
        el.appendEvent(rec);
        if (storage) {
            storage->appendRecord(rec);
        }
    }

    if (storage) {
        storage->flush();
    }

    size_t count = preparedBatch.size();
    TransactionRecord rec;
    rec.txId = ctx.txId;
    rec.startTimestamp = ctx.startTimestamp;
    rec.commitTimestamp = commitTimestamp;
    rec.committed = true;
    rec.operationCount = count;
    rec.author = ctx.author;
    m_history.push_back(std::move(rec));

    m_activeContext.reset();
    return Result<size_t>::ok(count);
}

} // namespace ttkv
