#pragma once

#include "Common.h"
#include "DataModel.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "StorageEngine.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cstdint>

namespace ttkv {

struct TxOperation {
    OpType opType{OpType::OP_SET};
    std::string key;
    std::string value;
    uint64_t expiryTimestamp{0};
    std::string description;
};

struct TxUndoEntry {
    std::string key;
    std::optional<std::string> previousStagedValue;
    bool wasStagedDeleted{false};
    bool existedInStaged{false};
};

struct TransactionRecord {
    uint64_t txId{0};
    uint64_t startTimestamp{0};
    uint64_t commitTimestamp{0};
    bool committed{false};
    size_t operationCount{0};
    std::string author;
};

class TransactionManager {
public:
    TransactionManager();

    bool isInTransaction() const { return m_activeContext.has_value(); }
    uint64_t currentTxId() const { return m_activeContext ? m_activeContext->txId : 0; }

    // Begins an atomic transaction
    Result<uint64_t> begin(uint64_t timestamp, std::string author = "system");

    // Stages operations within the active transaction
    Result<void> stageWrite(const std::string& key,
                            const std::string& value,
                            uint64_t expiry = 0,
                            std::string desc = "");

    Result<void> stageDelete(const std::string& key,
                             std::string desc = "");

    // Read-your-own-writes: checks staged overlay before looking into committed store
    std::optional<QueryResult> readStaged(const std::string& key) const;

    // Aborts transaction and unwinds using the Rollback Stack
    Result<void> abort();

    // Commits all staged operations atomically to KeyManager, EventLog, and StorageEngine
    Result<size_t> commit(KeyManager& km,
                          EventLog& el,
                          StorageEngine* storage,
                          uint64_t commitTimestamp);

    // Query historical transactions
    const std::vector<TransactionRecord>& transactionHistory() const { return m_history; }

private:
    struct ActiveContext {
        uint64_t txId{0};
        uint64_t startTimestamp{0};
        std::string author;
        std::vector<TxOperation> operations;
        std::unordered_map<std::string, std::string> stagedWrites;
        std::unordered_set<std::string> stagedDeletions;
        std::stack<TxUndoEntry> rollbackStack; // Stack data structure for undo
    };

    std::optional<ActiveContext> m_activeContext;
    std::vector<TransactionRecord> m_history;
    uint64_t m_nextTxId{1};
};

} // namespace ttkv
