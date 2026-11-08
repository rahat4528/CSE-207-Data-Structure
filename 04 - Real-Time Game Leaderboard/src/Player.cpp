#include "Player.h"
#include <numeric>

namespace LeaderboardEngine {

Player::Player(std::string id, std::string username, timestamp_t registeredAt)
    : id_(std::move(id)), username_(std::move(username)), registeredAt_(registeredAt) {}

void Player::setUsername(const std::string& newUsername) {
    username_ = newUsername;
}

void Player::recordScoreUpdate(score_t newScore, timestamp_t time, const std::string& eventId, size_t maxHistory) {
    score_t delta = newScore - currentScore_;
    currentScore_ = newScore;
    if (updateCount_ == 0 || newScore > bestScore_) {
        bestScore_ = newScore;
    }
    if (updateCount_ == 0) {
        firstUpdateTime_ = time;
    }
    lastUpdateTime_ = time;
    updateCount_++;

    scoreHistory_.push_back({newScore, delta, time, eventId});
    if (scoreHistory_.size() > maxHistory) {
        scoreHistory_.erase(scoreHistory_.begin());
    }
}

void Player::recordRankSnapshot(size_t rank, timestamp_t time, size_t maxHistory) {
    rankHistory_.push_back({rank, currentScore_, time});
    if (rankHistory_.size() > maxHistory) {
        rankHistory_.erase(rankHistory_.begin());
    }
}

double Player::calculateMovingAverage(size_t windowSize) const {
    if (scoreHistory_.empty() || windowSize == 0) return static_cast<double>(currentScore_);
    size_t count = std::min(windowSize, scoreHistory_.size());
    int64_t sum = 0;
    for (size_t i = scoreHistory_.size() - count; i < scoreHistory_.size(); ++i) {
        sum += scoreHistory_[i].score;
    }
    return static_cast<double>(sum) / static_cast<double>(count);
}

int64_t Player::calculateScoreTrend() const {
    if (scoreHistory_.size() < 2) return 0;
    return scoreHistory_.back().score - scoreHistory_.front().score;
}

} // namespace LeaderboardEngine
