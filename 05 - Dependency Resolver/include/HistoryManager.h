#pragma once

#include "Common.h"
#include "TransactionManager.h"
#include <string>
#include <vector>
#include <memory>

namespace DepResolver {

struct TransactionRecord {
    uint64_t transactionId{0};
    std::string timestamp;
    std::string actionName; // e.g. "install", "upgrade", "rollback"
    TransactionStatus status{TransactionStatus::COMMITTED};
    std::vector<std::string> affectedPackages;
    std::string summary;
    std::vector<SimLogEntry> logs;
};

// ============================================================================
// Custom Doubly-Linked List for Transaction History (Data Structure Requirement)
// ============================================================================
struct HistoryNode {
    TransactionRecord record;
    HistoryNode* prev{nullptr};
    HistoryNode* next{nullptr};

    explicit HistoryNode(TransactionRecord rec) : record(std::move(rec)) {}
};

class HistoryManager {
private:
    HistoryNode* head_{nullptr};
    HistoryNode* tail_{nullptr};
    size_t count_{0};

public:
    HistoryManager();
    ~HistoryManager();

    // Prevent copying
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    void append(TransactionRecord record);

    const TransactionRecord* findById(uint64_t id) const;
    std::vector<const TransactionRecord*> searchByPackage(const std::string& pkgName) const;
    std::vector<TransactionRecord> getAll() const;

    size_t size() const { return count_; }
    void clear();
};

} // namespace DepResolver

