#ifndef COORDINATE_COMPRESSOR_H
#define COORDINATE_COMPRESSOR_H

#include "Common.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstddef>

namespace LeaderboardEngine {

class CoordinateCompressor {
public:
    explicit CoordinateCompressor(size_t rebuildThreshold = 128);

    // Initial domain setup or complete rebuild from active score distribution
    void build(const std::vector<score_t>& scores);

    // Lookup 1-based index for a known score in the compressed domain.
    // Returns 0 if score is not currently in the compressed domain.
    size_t getIndex(score_t score) const;

    // Reverse lookup: 1-based index to score value
    score_t getScore(size_t index) const;

    bool contains(score_t score) const;

    // Tracks an online score into pending buffer if not present
    void addPending(score_t score);
    void removePending(score_t score);

    // Count of scores in pending buffer strictly greater than given score
    int64_t countPendingGreater(score_t score) const;
    int64_t countPendingEqual(score_t score) const;

    bool needsRebuild() const;
    size_t domainSize() const { return distinctScores_.size(); }
    size_t pendingCount() const { return pendingBuffer_.size(); }
    void clear();

    const std::vector<score_t>& distinctScores() const { return distinctScores_; }

private:
    size_t rebuildThreshold_;
    std::vector<score_t> distinctScores_; // strictly sorted ascending
    std::vector<score_t> pendingBuffer_;  // unsorted or small sorted pending scores
};

} // namespace LeaderboardEngine

#endif // COORDINATE_COMPRESSOR_H
