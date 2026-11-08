#include "Leaderboard.h"
#include <algorithm>

namespace LeaderboardEngine {

Leaderboard::Leaderboard(std::string boardId, std::string category, TimeWindow window)
    : boardId_(std::move(boardId)), category_(std::move(category)), window_(window), compressor_(128) {}

size_t Leaderboard::playerCount() const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    return players_.size();
}

size_t Leaderboard::computeRankUnderLock(score_t score) const {
    int64_t countInFenwick = 0;
    const auto& distinct = compressor_.distinctScores();

    if (!distinct.empty()) {
        auto it = std::upper_bound(distinct.begin(), distinct.end(), score);
        if (it != distinct.end()) {
            size_t idx = static_cast<size_t>(std::distance(distinct.begin(), it) + 1);
            // Count all scores from idx to end of domain
            countInFenwick = fenwick_.total() - fenwick_.prefix(idx - 1);
        } else {
            countInFenwick = 0; // Score is strictly greater than all distinct scores in Fenwick
        }
    }

    int64_t countInPending = compressor_.countPendingGreater(score);
    return static_cast<size_t>(1 + countInFenwick + countInPending);
}

void Leaderboard::rebuildIndexes() {
    std::vector<score_t> allScores;
    allScores.reserve(players_.size());
    for (const auto& kv : players_) {
        allScores.push_back(kv.second.score);
    }
    compressor_.build(allScores);
    fenwick_.resize(compressor_.domainSize());

    for (const auto& kv : players_) {
        size_t idx = compressor_.getIndex(kv.second.score);
        if (idx > 0) {
            fenwick_.add(idx, 1);
        }
    }
}

void Leaderboard::applyScoreChangeUnderLock(const std::string& playerId, score_t oldScore, bool hadOldScore, score_t newScore, timestamp_t time) {
    if (hadOldScore) {
        size_t oldIdx = compressor_.getIndex(oldScore);
        if (oldIdx > 0) {
            fenwick_.add(oldIdx, -1);
        } else {
            compressor_.removePending(oldScore);
        }
    }

    size_t newIdx = compressor_.getIndex(newScore);
    if (newIdx > 0) {
        fenwick_.add(newIdx, 1);
    } else {
        compressor_.addPending(newScore);
    }

    heap_.update(playerId, newScore, time);

    if (compressor_.needsRebuild()) {
        rebuildIndexes();
    }
}

Result<ScoreUpdateResult> Leaderboard::updateScore(const ScoreEvent& event) {
    std::unique_lock<std::shared_mutex> lock(rwLock_);

    score_t oldScore = 0;
    bool hadOldScore = false;
    size_t oldRank = 0;

    auto it = players_.find(event.playerId);
    if (it != players_.end()) {
        hadOldScore = true;
        oldScore = it->second.score;
        oldRank = computeRankUnderLock(oldScore);
    }

    score_t newScore = oldScore;
    if (event.mode == ScoreUpdateMode::SET) {
        newScore = event.value;
    } else if (event.mode == ScoreUpdateMode::ADD) {
        newScore += event.value;
    } else if (event.mode == ScoreUpdateMode::MAX) {
        if (hadOldScore && event.value <= oldScore) {
            // No update needed for MAX mode
            return Result<ScoreUpdateResult>::Success({event.playerId, oldScore, oldScore, oldRank, oldRank, version_, false});
        }
        newScore = event.value;
    }

    // Apply state change
    players_[event.playerId] = {event.playerId, newScore, event.timestamp};
    applyScoreChangeUnderLock(event.playerId, oldScore, hadOldScore, newScore, event.timestamp);

    size_t newRank = computeRankUnderLock(newScore);
    version_++;

    ScoreUpdateResult res;
    res.playerId = event.playerId;
    res.oldScore = oldScore;
    res.newScore = newScore;
    res.oldRank = hadOldScore ? oldRank : 0;
    res.newRank = newRank;
    res.boardVersion = version_;
    res.rankChanged = (!hadOldScore || oldRank != newRank);

    return Result<ScoreUpdateResult>::Success(res);
}

Result<void> Leaderboard::removePlayer(const std::string& playerId) {
    std::unique_lock<std::shared_mutex> lock(rwLock_);
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return Result<void>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not found on this leaderboard");
    }

    score_t oldScore = it->second.score;
    size_t oldIdx = compressor_.getIndex(oldScore);
    if (oldIdx > 0) {
        fenwick_.add(oldIdx, -1);
    } else {
        compressor_.removePending(oldScore);
    }

    heap_.remove(playerId);
    players_.erase(it);
    version_++;

    return Result<void>::Success();
}

Result<size_t> Leaderboard::getRank(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return Result<size_t>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not on leaderboard");
    }
    return Result<size_t>::Success(computeRankUnderLock(it->second.score));
}

Result<size_t> Leaderboard::getDenseRank(score_t score) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    std::vector<score_t> distinctAbove;
    for (const auto& kv : players_) {
        if (kv.second.score > score) {
            distinctAbove.push_back(kv.second.score);
        }
    }
    std::sort(distinctAbove.begin(), distinctAbove.end());
    distinctAbove.erase(std::unique(distinctAbove.begin(), distinctAbove.end()), distinctAbove.end());
    return Result<size_t>::Success(1 + distinctAbove.size());
}

Result<double> Leaderboard::getPercentile(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return Result<double>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not on leaderboard");
    }
    if (players_.empty()) return Result<double>::Success(100.0);
    size_t rank = computeRankUnderLock(it->second.score);
    double pct = (static_cast<double>(players_.size() - rank) / static_cast<double>(players_.size())) * 100.0;
    return Result<double>::Success(std::max(0.0, pct));
}

Result<score_t> Leaderboard::getPointsToNextRank(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return Result<score_t>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not on leaderboard");
    }
    score_t myScore = it->second.score;
    score_t nextScore = myScore;
    bool found = false;

    for (const auto& kv : players_) {
        if (kv.second.score > myScore) {
            if (!found || kv.second.score < nextScore) {
                nextScore = kv.second.score;
                found = true;
            }
        }
    }

    if (!found) {
        return Result<score_t>::Success(0); // Already rank 1
    }
    return Result<score_t>::Success(nextScore - myScore);
}

std::vector<LeaderboardEntry> Leaderboard::getTopK(size_t k) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    std::vector<LeaderboardEntry> entries;
    auto topHeap = heap_.getTopK(k);
    entries.reserve(topHeap.size());

    for (const auto& item : topHeap) {
        size_t rank = computeRankUnderLock(item.score);
        double pct = players_.empty() ? 100.0 : ((double)(players_.size() - rank) / (double)players_.size()) * 100.0;
        entries.push_back({rank, item.playerId, item.score, item.timestamp, std::max(0.0, pct)});
    }
    return entries;
}

std::vector<LeaderboardEntry> Leaderboard::getPage(size_t offset, size_t limit) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    std::vector<LeaderboardEntry> entries;
    if (limit == 0 || offset >= players_.size()) {
        return entries;
    }
    // Extract top (offset + limit) and slice
    auto topHeap = heap_.getTopK(offset + limit);
    for (size_t i = offset; i < topHeap.size(); ++i) {
        const auto& item = topHeap[i];
        size_t rank = computeRankUnderLock(item.score);
        double pct = players_.empty() ? 100.0 : ((double)(players_.size() - rank) / (double)players_.size()) * 100.0;
        entries.push_back({rank, item.playerId, item.score, item.timestamp, std::max(0.0, pct)});
    }
    return entries;
}

std::vector<LeaderboardEntry> Leaderboard::getPlayersAround(const std::string& playerId, size_t radius) const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);
    std::vector<LeaderboardEntry> entries;
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return entries;
    }

    size_t myRank = computeRankUnderLock(it->second.score);
    size_t startRank = (myRank > radius) ? (myRank - radius) : 1;
    size_t offset = startRank - 1;
    size_t limit = (2 * radius) + 1;

    return getPage(offset, limit);
}

bool Leaderboard::validateInvariant() const {
    std::shared_lock<std::shared_mutex> lock(rwLock_);

    // Invariant 1: Total counts must match
    int64_t totalInTree = fenwick_.total() + compressor_.pendingCount();
    if (static_cast<size_t>(totalInTree) != players_.size()) {
        return false;
    }
    if (heap_.size() != players_.size()) {
        return false;
    }
    if (!heap_.validateHeapInvariant()) {
        return false;
    }

    // Invariant 2: Brute-force verification of every player's rank
    for (const auto& kv : players_) {
        size_t fenwickRank = computeRankUnderLock(kv.second.score);
        size_t bruteRank = 1;
        for (const auto& other : players_) {
            if (other.second.score > kv.second.score) {
                bruteRank++;
            }
        }
        if (fenwickRank != bruteRank) {
            return false;
        }
    }

    return true;
}

} // namespace LeaderboardEngine
