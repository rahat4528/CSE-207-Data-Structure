#include "RateLimiter.h"
#include <algorithm>

namespace LeaderboardEngine {

RateLimiter::RateLimiter(double tokensPerSecond, double maxBurst, std::shared_ptr<IClock> clock)
    : tokensPerSecond_(tokensPerSecond), maxBurst_(maxBurst), clock_(std::move(clock)) {}

bool RateLimiter::allowRequest(const std::string& playerId) {
    std::lock_guard<std::mutex> lock(lock_);
    timestamp_t now = clock_->nowMs();

    auto it = buckets_.find(playerId);
    if (it == buckets_.end()) {
        buckets_[playerId] = {maxBurst_ - 1.0, now};
        return true;
    }

    Bucket& b = it->second;
    double elapsedSec = static_cast<double>(now - b.lastRefillMs) / 1000.0;
    if (elapsedSec > 0.0) {
        b.tokens = std::min(maxBurst_, b.tokens + elapsedSec * tokensPerSecond_);
        b.lastRefillMs = now;
    }

    if (b.tokens >= 1.0) {
        b.tokens -= 1.0;
        return true;
    }

    return false; // Rate limit exceeded
}

Result<void> RateLimiter::validateScoreJump(score_t currentScore, score_t newScore, score_t maxAllowedJump) {
    if (newScore < 0) {
        return Result<void>::Failure(ErrorCode::SCORE_OUT_OF_BOUNDS, "Negative scores are not permitted");
    }
    score_t delta = newScore - currentScore;
    if (delta > maxAllowedJump) {
        return Result<void>::Failure(ErrorCode::SCORE_JUMP_TOO_LARGE,
            "Score jump (" + std::to_string(delta) + ") exceeds anti-cheat maximum limit (" + std::to_string(maxAllowedJump) + ")");
    }
    return Result<void>::Success();
}

} // namespace LeaderboardEngine
