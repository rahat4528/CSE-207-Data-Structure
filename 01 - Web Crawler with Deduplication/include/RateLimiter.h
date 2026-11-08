#ifndef CRAWLER_RATE_LIMITER_H
#define CRAWLER_RATE_LIMITER_H

#include "Common.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>

namespace crawler {

class RateLimiter {
public:
    explicit RateLimiter(uint32_t defaultDelayMs = 1000);

    // Determines how long to wait before next request to host (returns ms to wait)
    uint32_t getWaitTimeMs(const std::string& host, uint32_t customDelayMs = 0);

    // Records that a request was just dispatched to host
    void recordRequest(const std::string& host);

    // Calculates exponential backoff with jitter
    static uint32_t calculateBackoffMs(uint32_t retryAttempt, 
                                      uint32_t baseDelayMs = 1000, 
                                      std::optional<uint32_t> retryAfterSec = std::nullopt);

    void clear();

private:
    uint32_t m_defaultDelayMs{1000};
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_lastRequest;
    std::mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_RATE_LIMITER_H

