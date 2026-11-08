#include "IndexedMaxHeap.h"
#include <queue>

namespace LeaderboardEngine {

IndexedMaxHeap::IndexedMaxHeap() = default;

void IndexedMaxHeap::swapEntries(size_t i, size_t j) {
    std::swap(heap_[i], heap_[j]);
    positionMap_[heap_[i].playerId] = i;
    positionMap_[heap_[j].playerId] = j;
}

void IndexedMaxHeap::siftUp(size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (heap_[index] > heap_[parent]) {
            swapEntries(index, parent);
            index = parent;
        } else {
            break;
        }
    }
}

void IndexedMaxHeap::siftDown(size_t index) {
    size_t size = heap_.size();
    while (true) {
        size_t largest = index;
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;

        if (left < size && heap_[left] > heap_[largest]) {
            largest = left;
        }
        if (right < size && heap_[right] > heap_[largest]) {
            largest = right;
        }

        if (largest != index) {
            swapEntries(index, largest);
            index = largest;
        } else {
            break;
        }
    }
}

void IndexedMaxHeap::insert(const std::string& playerId, score_t score, timestamp_t timestamp) {
    if (positionMap_.find(playerId) != positionMap_.end()) {
        update(playerId, score, timestamp);
        return;
    }
    size_t newIndex = heap_.size();
    heap_.push_back({playerId, score, timestamp});
    positionMap_[playerId] = newIndex;
    siftUp(newIndex);
}

void IndexedMaxHeap::update(const std::string& playerId, score_t score, timestamp_t timestamp) {
    auto it = positionMap_.find(playerId);
    if (it == positionMap_.end()) {
        insert(playerId, score, timestamp);
        return;
    }
    size_t index = it->second;
    HeapEntry old = heap_[index];
    heap_[index] = {playerId, score, timestamp};

    if (heap_[index] > old) {
        siftUp(index);
    } else {
        siftDown(index);
    }
}

void IndexedMaxHeap::remove(const std::string& playerId) {
    auto it = positionMap_.find(playerId);
    if (it == positionMap_.end()) {
        return;
    }
    size_t index = it->second;
    size_t lastIndex = heap_.size() - 1;

    if (index != lastIndex) {
        swapEntries(index, lastIndex);
    }
    positionMap_.erase(playerId);
    heap_.pop_back();

    if (index < heap_.size()) {
        siftUp(index);
        siftDown(index);
    }
}

bool IndexedMaxHeap::contains(const std::string& playerId) const {
    return positionMap_.find(playerId) != positionMap_.end();
}

const HeapEntry& IndexedMaxHeap::top() const {
    if (heap_.empty()) {
        throw std::runtime_error("IndexedMaxHeap: top() called on empty heap");
    }
    return heap_[0];
}

HeapEntry IndexedMaxHeap::pop() {
    if (heap_.empty()) {
        throw std::runtime_error("IndexedMaxHeap: pop() called on empty heap");
    }
    HeapEntry root = heap_[0];
    remove(root.playerId);
    return root;
}

std::vector<HeapEntry> IndexedMaxHeap::getTopK(size_t k) const {
    std::vector<HeapEntry> result;
    if (k == 0 || heap_.empty()) {
        return result;
    }
    k = std::min(k, heap_.size());
    result.reserve(k);

    // Frontier node for O(K log K) exploration
    struct FrontierNode {
        HeapEntry entry;
        size_t heapIndex;

        bool operator<(const FrontierNode& other) const {
            return entry < other.entry; // Max heap for priority_queue
        }
    };

    std::priority_queue<FrontierNode> frontier;
    frontier.push({heap_[0], 0});

    while (!frontier.empty() && result.size() < k) {
        FrontierNode current = frontier.top();
        frontier.pop();

        result.push_back(current.entry);

        size_t left = 2 * current.heapIndex + 1;
        size_t right = 2 * current.heapIndex + 2;

        if (left < heap_.size()) {
            frontier.push({heap_[left], left});
        }
        if (right < heap_.size()) {
            frontier.push({heap_[right], right});
        }
    }

    return result;
}

void IndexedMaxHeap::clear() {
    heap_.clear();
    positionMap_.clear();
}

bool IndexedMaxHeap::validateHeapInvariant() const {
    for (size_t i = 0; i < heap_.size(); ++i) {
        auto it = positionMap_.find(heap_[i].playerId);
        if (it == positionMap_.end() || it->second != i) {
            return false;
        }
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;
        if (left < heap_.size() && heap_[left] > heap_[i]) {
            return false;
        }
        if (right < heap_.size() && heap_[right] > heap_[i]) {
            return false;
        }
    }
    return true;
}

} // namespace LeaderboardEngine
