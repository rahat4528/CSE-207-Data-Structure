#include "RateLimiter.h"
#include <algorithm>

namespace crawler {

RateLimiter::RateLimiter(uint32_t defaultDelayMs) : m_defaultDelayMs(defaultDelayMs) {}

uint32_t RateLimiter::getWaitTimeMs(const std::string& host, uint32_t customDelayMs) {
    uint32_t requiredDelay = (customDelayMs > 0) ? customDelayMs : m_defaultDelayMs;
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_lastRequest.find(host);
    if (it == m_lastRequest.end()) {
        return 0; // First request to this host: no wait
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    if (elapsed >= requiredDelay) {
        return 0;
    }
    return static_cast<uint32_t>(requiredDelay - elapsed);
}

void RateLimiter::recordRequest(const std::string& host) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastRequest[host] = std::chrono::steady_clock::now();
}

uint32_t RateLimiter::calculateBackoffMs(uint32_t retryAttempt, 
                                        uint32_t baseDelayMs, 
                                        std::optional<uint32_t> retryAfterSec) {
    if (retryAfterSec.has_value()) {
        return retryAfterSec.value() * 1000;
    }

    // Exponential backoff: base * (2 ^ attempt) capped at 60s
    uint32_t multiplier = 1u << std::min<uint32_t>(retryAttempt, 6u);
    uint32_t backoff = baseDelayMs * multiplier;
    // Add deterministic pseudo-jitter (50ms - 200ms)
    uint32_t jitter = (retryAttempt * 37) % 150 + 50;
    return std::min<uint32_t>(backoff + jitter, 60000u);
}

void RateLimiter::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastRequest.clear();
}

} // namespace crawler

