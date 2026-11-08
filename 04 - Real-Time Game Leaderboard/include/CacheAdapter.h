#ifndef CACHE_ADAPTER_H
#define CACHE_ADAPTER_H

#include "Common.h"
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <memory>

namespace LeaderboardEngine {

class ICacheAdapter {
public:
    virtual ~ICacheAdapter() = default;

    virtual void set(const std::string& key, const std::string& value, int ttlSeconds) = 0;
    virtual bool get(const std::string& key, std::string& outValue) = 0;
    virtual void invalidate(const std::string& key) = 0;
    virtual void invalidatePrefix(const std::string& prefix) = 0;
    virtual void flush() = 0;

    virtual bool isHealthy() const = 0;
    virtual size_t hitCount() const = 0;
    virtual size_t missCount() const = 0;
};

class InMemoryCacheAdapter : public ICacheAdapter {
public:
    explicit InMemoryCacheAdapter(std::shared_ptr<IClock> clock = std::make_shared<SystemClock>());

    void set(const std::string& key, const std::string& value, int ttlSeconds) override;
    bool get(const std::string& key, std::string& outValue) override;
    void invalidate(const std::string& key) override;
    void invalidatePrefix(const std::string& prefix) override;
    void flush() override;

    bool isHealthy() const override { return true; }
    size_t hitCount() const override { return hits_; }
    size_t missCount() const override { return misses_; }

private:
    struct CacheItem {
        std::string value;
        timestamp_t expiresAt;
    };

    std::shared_ptr<IClock> clock_;
    mutable std::shared_mutex lock_;
    std::unordered_map<std::string, CacheItem> store_;
    size_t hits_{0};
    size_t misses_{0};
};

} // namespace LeaderboardEngine

#endif // CACHE_ADAPTER_H
