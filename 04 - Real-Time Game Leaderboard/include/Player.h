#ifndef PLAYER_H
#define PLAYER_H

#include "Common.h"
#include <string>
#include <vector>

namespace LeaderboardEngine {

struct ScoreRecord {
    score_t score;
    score_t delta;
    timestamp_t timestamp;
    std::string eventId;
};

struct RankRecord {
    size_t rank;
    score_t score;
    timestamp_t timestamp;
};

class Player {
public:
    Player() = default;
    Player(std::string id, std::string username, timestamp_t registeredAt);

    const std::string& id() const { return id_; }
    const std::string& username() const { return username_; }
    timestamp_t registeredAt() const { return registeredAt_; }

    score_t currentScore() const { return currentScore_; }
    score_t bestScore() const { return bestScore_; }
    size_t updateCount() const { return updateCount_; }
    timestamp_t firstUpdateTime() const { return firstUpdateTime_; }
    timestamp_t lastUpdateTime() const { return lastUpdateTime_; }

    const std::vector<ScoreRecord>& scoreHistory() const { return scoreHistory_; }
    const std::vector<RankRecord>& rankHistory() const { return rankHistory_; }

    void recordScoreUpdate(score_t newScore, timestamp_t time, const std::string& eventId, size_t maxHistory = 100);
    void recordRankSnapshot(size_t rank, timestamp_t time, size_t maxHistory = 100);
    void setUsername(const std::string& newUsername);

    // Moving average and performance trend calculations
    double calculateMovingAverage(size_t windowSize = 5) const;
    int64_t calculateScoreTrend() const; // difference between average of last half and first half

private:
    std::string id_;
    std::string username_;
    timestamp_t registeredAt_{0};

    score_t currentScore_{0};
    score_t bestScore_{0};
    size_t updateCount_{0};
    timestamp_t firstUpdateTime_{0};
    timestamp_t lastUpdateTime_{0};

    std::vector<ScoreRecord> scoreHistory_;
    std::vector<RankRecord> rankHistory_;
};

} // namespace LeaderboardEngine

#endif // PLAYER_H
