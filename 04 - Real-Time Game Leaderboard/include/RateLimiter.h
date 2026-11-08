#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include "Common.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace LeaderboardEngine {

class RateLimiter {
public:
    RateLimiter(double tokensPerSecond, double maxBurst, std::shared_ptr<IClock> clock = std::make_shared<SystemClock>());

    bool allowRequest(const std::string& playerId);
    Result<void> validateScoreJump(score_t currentScore, score_t newScore, score_t maxAllowedJump = 5000);

private:
    struct Bucket {
        double tokens;
        timestamp_t lastRefillMs;
    };

    double tokensPerSecond_;
    double maxBurst_;
    std::shared_ptr<IClock> clock_;
    mutable std::mutex lock_;
    std::unordered_map<std::string, Bucket> buckets_;
};

} // namespace LeaderboardEngine

#endif // RATE_LIMITER_H
