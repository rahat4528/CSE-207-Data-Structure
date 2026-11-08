#ifndef INDEXED_MAX_HEAP_H
#define INDEXED_MAX_HEAP_H

#include "Common.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>

namespace LeaderboardEngine {

struct HeapEntry {
    std::string playerId;
    score_t score;
    timestamp_t timestamp;

    // Strict Total Order:
    // 1. Higher score wins
    // 2. Earlier timestamp wins
    // 3. Smaller player ID wins
    bool operator>(const HeapEntry& other) const {
        if (score != other.score) return score > other.score;
        if (timestamp != other.timestamp) return timestamp < other.timestamp;
        return playerId < other.playerId;
    }

    bool operator<(const HeapEntry& other) const {
        if (score != other.score) return score < other.score;
        if (timestamp != other.timestamp) return timestamp > other.timestamp;
        return playerId > other.playerId;
    }

    bool operator==(const HeapEntry& other) const {
        return playerId == other.playerId && score == other.score && timestamp == other.timestamp;
    }
};

class IndexedMaxHeap {
public:
    IndexedMaxHeap();

    void insert(const std::string& playerId, score_t score, timestamp_t timestamp);
    void update(const std::string& playerId, score_t score, timestamp_t timestamp);
    void remove(const std::string& playerId);

    bool contains(const std::string& playerId) const;
    const HeapEntry& top() const;
    HeapEntry pop();

    // Returns Top-K elements without destroying the heap (Auxiliary frontier heap approach O(K log K))
    std::vector<HeapEntry> getTopK(size_t k) const;

    size_t size() const { return heap_.size(); }
    bool empty() const { return heap_.empty(); }
    void clear();

    // Invariant verification for tests and debug mode
    bool validateHeapInvariant() const;

private:
    std::vector<HeapEntry> heap_;
    std::unordered_map<std::string, size_t> positionMap_; // playerId -> index in heap_

    void siftUp(size_t index);
    void siftDown(size_t index);
    void swapEntries(size_t i, size_t j);
};

} // namespace LeaderboardEngine

#endif // INDEXED_MAX_HEAP_H
