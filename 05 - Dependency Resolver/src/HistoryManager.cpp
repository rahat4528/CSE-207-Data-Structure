#include "HistoryManager.h"
#include <algorithm>

namespace DepResolver {

HistoryManager::HistoryManager() : head_(nullptr), tail_(nullptr), count_(0) {}

HistoryManager::~HistoryManager() {
    clear();
}

void HistoryManager::clear() {
    HistoryNode* curr = head_;
    while (curr) {
        HistoryNode* next = curr->next;
        delete curr;
        curr = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    count_ = 0;
}

void HistoryManager::append(TransactionRecord record) {
    HistoryNode* node = new HistoryNode(std::move(record));
    if (!head_) {
        head_ = node;
        tail_ = node;
    } else {
        tail_->next = node;
        node->prev = tail_;
        tail_ = node;
    }
    count_++;
}

const TransactionRecord* HistoryManager::findById(uint64_t id) const {
    HistoryNode* curr = head_;
    while (curr) {
        if (curr->record.transactionId == id) {
            return &curr->record;
        }
        curr = curr->next;
    }
    return nullptr;
}

std::vector<const TransactionRecord*> HistoryManager::searchByPackage(const std::string& pkgName) const {
    std::vector<const TransactionRecord*> results;
    HistoryNode* curr = head_;
    while (curr) {
        for (const auto& pkg : curr->record.affectedPackages) {
            if (pkg == pkgName) {
                results.push_back(&curr->record);
                break;
            }
        }
        curr = curr->next;
    }
    return results;
}

std::vector<TransactionRecord> HistoryManager::getAll() const {
    std::vector<TransactionRecord> list;
    list.reserve(count_);
    HistoryNode* curr = head_;
    while (curr) {
        list.push_back(curr->record);
        curr = curr->next;
    }
    return list;
}

} // namespace DepResolver

