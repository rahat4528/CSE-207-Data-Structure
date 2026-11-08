#ifndef CRAWLER_FAKE_FETCHER_H
#define CRAWLER_FAKE_FETCHER_H

#include "IFetcher.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

namespace crawler {

struct FakeEndpoint {
    int statusCode{200};
    std::string statusText{"OK"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool timeout{false};
    uint32_t delayMs{0};
    std::optional<uint32_t> retryAfterSec;
};

class FakeFetcher : public IFetcher {
public:
    FakeFetcher() = default;

    void registerResponse(const std::string& url, 
                          int statusCode, 
                          const std::string& body, 
                          std::unordered_map<std::string, std::string> headers = {});

    void registerRedirect(const std::string& fromUrl, 
                          const std::string& toUrl, 
                          int statusCode = 301);

    void registerRedirectLoop(const std::string& urlA, 
                              const std::string& urlB);

    void registerTimeout(const std::string& url);

    void registerOversized(const std::string& url, size_t sizeBytes);

    void registerRateLimit(const std::string& url, uint32_t retryAfterSec);

    void registerRobotsTxt(const std::string& origin, const std::string& content);

    Result<FetchResponse> fetch(const FetchRequest& req) override;

    size_t fetchCount(const std::string& url) const;
    size_t totalFetches() const;
    void resetStats();

private:
    std::unordered_map<std::string, FakeEndpoint> m_endpoints;
    mutable std::unordered_map<std::string, size_t> m_fetchCounts;
    mutable size_t m_totalFetches{0};
    mutable std::mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_FAKE_FETCHER_H

