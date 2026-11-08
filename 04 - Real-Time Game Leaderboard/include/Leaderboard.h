#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include "Common.h"
#include "ScoreEvent.h"
#include "FenwickTree.h"
#include "CoordinateCompressor.h"
#include "IndexedMaxHeap.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <shared_mutex>
#include <mutex>

namespace LeaderboardEngine {

struct LeaderboardEntry {
    size_t rank;
    std::string playerId;
    score_t score;
    timestamp_t timestamp;
    double percentile;
};

struct BoardPlayerRecord {
    std::string playerId;
    score_t score{0};
    timestamp_t lastUpdated{0};
};

class Leaderboard {
public:
    Leaderboard(std::string boardId, std::string category, TimeWindow window);

    const std::string& boardId() const { return boardId_; }
    const std::string& category() const { return category_; }
    TimeWindow window() const { return window_; }
    uint64_t version() const { return version_; }
    size_t playerCount() const;

    // Thread-safe atomic score update
    Result<ScoreUpdateResult> updateScore(const ScoreEvent& event);

    // Remove player (e.g. ban or score reset)
    Result<void> removePlayer(const std::string& playerId);

    // Queries
    Result<size_t> getRank(const std::string& playerId) const;
    Result<size_t> getDenseRank(score_t score) const;
    Result<double> getPercentile(const std::string& playerId) const;
    Result<score_t> getPointsToNextRank(const std::string& playerId) const;

    std::vector<LeaderboardEntry> getTopK(size_t k) const;
    std::vector<LeaderboardEntry> getPage(size_t offset, size_t limit) const;
    std::vector<LeaderboardEntry> getPlayersAround(const std::string& playerId, size_t radius) const;

    // Full Invariant Consistency Checker
    bool validateInvariant() const;

    // Rebuild coordinate compression and Fenwick index
    void rebuildIndexes();

private:
    std::string boardId_;
    std::string category_;
    TimeWindow window_;
    uint64_t version_{1};

    mutable std::shared_mutex rwLock_;

    std::unordered_map<std::string, BoardPlayerRecord> players_;
    CoordinateCompressor compressor_;
    FenwickTree fenwick_;
    IndexedMaxHeap heap_;

    // Internal non-locking helpers (called under lock)
    size_t computeRankUnderLock(score_t score) const;
    void applyScoreChangeUnderLock(const std::string& playerId, score_t oldScore, bool hadOldScore, score_t newScore, timestamp_t time);
};

} // namespace LeaderboardEngine

#endif // LEADERBOARD_H
