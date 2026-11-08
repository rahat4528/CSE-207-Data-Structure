#include "CacheAdapter.h"
#include <mutex>

namespace LeaderboardEngine {

InMemoryCacheAdapter::InMemoryCacheAdapter(std::shared_ptr<IClock> clock)
    : clock_(std::move(clock)) {}

void InMemoryCacheAdapter::set(const std::string& key, const std::string& value, int ttlSeconds) {
    std::unique_lock<std::shared_mutex> lock(lock_);
    timestamp_t expiresAt = (ttlSeconds > 0) ? (clock_->nowMs() + ttlSeconds * 1000) : 0;
    store_[key] = {value, expiresAt};
}

bool InMemoryCacheAdapter::get(const std::string& key, std::string& outValue) {
    std::shared_lock<std::shared_mutex> lock(lock_);
    auto it = store_.find(key);
    if (it == store_.end()) {
        const_cast<InMemoryCacheAdapter*>(this)->misses_++;
        return false;
    }

    timestamp_t now = clock_->nowMs();
    if (it->second.expiresAt > 0 && it->second.expiresAt < now) {
        // Expired
        const_cast<InMemoryCacheAdapter*>(this)->misses_++;
        return false;
    }

    outValue = it->second.value;
    const_cast<InMemoryCacheAdapter*>(this)->hits_++;
    return true;
}

void InMemoryCacheAdapter::invalidate(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(lock_);
    store_.erase(key);
}

void InMemoryCacheAdapter::invalidatePrefix(const std::string& prefix) {
    std::unique_lock<std::shared_mutex> lock(lock_);
    for (auto it = store_.begin(); it != store_.end(); ) {
        if (it->first.rfind(prefix, 0) == 0) { // starts_with
            it = store_.erase(it);
        } else {
            ++it;
        }
    }
}

void InMemoryCacheAdapter::flush() {
    std::unique_lock<std::shared_mutex> lock(lock_);
    store_.clear();
}

} // namespace LeaderboardEngine
