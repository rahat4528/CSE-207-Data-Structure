#include "CoordinateCompressor.h"
#include <stdexcept>

namespace LeaderboardEngine {

CoordinateCompressor::CoordinateCompressor(size_t rebuildThreshold)
    : rebuildThreshold_(rebuildThreshold) {}

void CoordinateCompressor::build(const std::vector<score_t>& scores) {
    distinctScores_ = scores;
    std::sort(distinctScores_.begin(), distinctScores_.end());
    distinctScores_.erase(std::unique(distinctScores_.begin(), distinctScores_.end()), distinctScores_.end());
    pendingBuffer_.clear();
}

size_t CoordinateCompressor::getIndex(score_t score) const {
    if (distinctScores_.empty()) return 0;
    auto it = std::lower_bound(distinctScores_.begin(), distinctScores_.end(), score);
    if (it != distinctScores_.end() && *it == score) {
        return static_cast<size_t>(std::distance(distinctScores_.begin(), it) + 1); // 1-based
    }
    return 0;
}

score_t CoordinateCompressor::getScore(size_t index) const {
    if (index == 0 || index > distinctScores_.size()) {
        throw std::out_of_range("CoordinateCompressor: index out of bounds");
    }
    return distinctScores_[index - 1];
}

bool CoordinateCompressor::contains(score_t score) const {
    return getIndex(score) != 0;
}

void CoordinateCompressor::addPending(score_t score) {
    if (!contains(score)) {
        pendingBuffer_.push_back(score);
    }
}

void CoordinateCompressor::removePending(score_t score) {
    auto it = std::find(pendingBuffer_.begin(), pendingBuffer_.end(), score);
    if (it != pendingBuffer_.end()) {
        pendingBuffer_.erase(it);
    }
}

int64_t CoordinateCompressor::countPendingGreater(score_t score) const {
    int64_t count = 0;
    for (score_t s : pendingBuffer_) {
        if (s > score) {
            count++;
        }
    }
    return count;
}

int64_t CoordinateCompressor::countPendingEqual(score_t score) const {
    int64_t count = 0;
    for (score_t s : pendingBuffer_) {
        if (s == score) {
            count++;
        }
    }
    return count;
}

bool CoordinateCompressor::needsRebuild() const {
    return pendingBuffer_.size() >= rebuildThreshold_;
}

void CoordinateCompressor::clear() {
    distinctScores_.clear();
    pendingBuffer_.clear();
}

} // namespace LeaderboardEngine
