#include "FakeFetcher.h"
#include "UrlNormalizer.h"
#include <algorithm>

namespace crawler {

void FakeFetcher::registerResponse(const std::string& url, 
                                   int statusCode, 
                                   const std::string& body, 
                                   std::unordered_map<std::string, std::string> headers) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto norm = UrlNormalizer::normalize(url);
    std::string key = norm.isOk() ? norm.value() : url;

    FakeEndpoint ep;
    ep.statusCode = statusCode;
    ep.statusText = (statusCode == 200) ? "OK" : 
                    (statusCode == 404) ? "Not Found" : 
                    (statusCode == 429) ? "Too Many Requests" : "Status";
    ep.body = body;
    ep.headers = std::move(headers);
    if (!ep.headers.contains("Content-Type")) {
        ep.headers["Content-Type"] = "text/html; charset=utf-8";
    }
    m_endpoints[key] = std::move(ep);
}

void FakeFetcher::registerRedirect(const std::string& fromUrl, 
                                   const std::string& toUrl, 
                                   int statusCode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto normFrom = UrlNormalizer::normalize(fromUrl);
    std::string key = normFrom.isOk() ? normFrom.value() : fromUrl;

    FakeEndpoint ep;
    ep.statusCode = statusCode;
    ep.statusText = "Moved";
    ep.headers["Location"] = toUrl;
    m_endpoints[key] = std::move(ep);
}

void FakeFetcher::registerRedirectLoop(const std::string& urlA, 
                                       const std::string& urlB) {
    registerRedirect(urlA, urlB, 301);
    registerRedirect(urlB, urlA, 301);
}

void FakeFetcher::registerTimeout(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto norm = UrlNormalizer::normalize(url);
    std::string key = norm.isOk() ? norm.value() : url;

    FakeEndpoint ep;
    ep.timeout = true;
    m_endpoints[key] = std::move(ep);
}

void FakeFetcher::registerOversized(const std::string& url, size_t sizeBytes) {
    std::string huge(sizeBytes, 'X');
    registerResponse(url, 200, huge);
}

void FakeFetcher::registerRateLimit(const std::string& url, uint32_t retryAfterSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto norm = UrlNormalizer::normalize(url);
    std::string key = norm.isOk() ? norm.value() : url;

    FakeEndpoint ep;
    ep.statusCode = 429;
    ep.statusText = "Too Many Requests";
    ep.retryAfterSec = retryAfterSec;
    ep.headers["Retry-After"] = std::to_string(retryAfterSec);
    m_endpoints[key] = std::move(ep);
}

void FakeFetcher::registerRobotsTxt(const std::string& origin, const std::string& content) {
    std::string robotsUrl = origin;
    if (robotsUrl.back() == '/') robotsUrl.pop_back();
    robotsUrl += "/robots.txt";
    registerResponse(robotsUrl, 200, content, {{"Content-Type", "text/plain"}});
}

Result<FetchResponse> FakeFetcher::fetch(const FetchRequest& req) {
    std::string currentUrl = req.url;
    std::vector<std::string> redirectChain;

    for (uint32_t hop = 0; hop <= req.maxRedirects; ++hop) {
        auto normRes = UrlNormalizer::normalize(currentUrl);
        std::string lookupKey = normRes.isOk() ? normRes.value() : currentUrl;

        FakeEndpoint ep;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_totalFetches++;
            m_fetchCounts[lookupKey]++;

            auto it = m_endpoints.find(lookupKey);
            if (it == m_endpoints.end()) {
                // Default to 404 Not Found if endpoint is unmapped
                ep.statusCode = 404;
                ep.statusText = "Not Found";
                ep.body = "<html><body>404 Not Found</body></html>";
            } else {
                ep = it->second;
            }
        }

        if (ep.timeout) {
            return Result<FetchResponse>::error(StatusCode::READ_TIMEOUT, 
                "Simulated timeout fetching " + currentUrl);
        }

        // Check if redirect
        if (ep.statusCode >= 300 && ep.statusCode < 400 && ep.headers.contains("Location")) {
            redirectChain.push_back(currentUrl);
            std::string location = ep.headers["Location"];
            auto resolvedTarget = UrlNormalizer::resolve(currentUrl, location);
            if (resolvedTarget.isError()) {
                return Result<FetchResponse>::error(resolvedTarget.code(), resolvedTarget.errorMessage());
            }
            std::string nextUrl = resolvedTarget.value();

            // Circular redirect detection
            if (std::find(redirectChain.begin(), redirectChain.end(), nextUrl) != redirectChain.end()) {
                return Result<FetchResponse>::error(StatusCode::MAX_REDIRECTS_EXCEEDED, 
                    "Circular redirect detected: " + nextUrl);
            }

            currentUrl = nextUrl;
            continue; // follow redirect
        }

        // Terminal response
        FetchResponse resp;
        resp.statusCode = ep.statusCode;
        resp.statusText = ep.statusText;
        resp.headers = ep.headers;
        resp.finalUrl = currentUrl;
        resp.redirectChain = std::move(redirectChain);
        resp.retryAfterSec = ep.retryAfterSec;
        resp.responseTimeMs = 1;

        if (resp.headers.contains("Content-Type")) {
            resp.contentType = resp.headers["Content-Type"];
        }

        // Response size cap enforcement
        if (ep.body.size() > req.maxSizeBytes) {
            resp.body = ep.body.substr(0, req.maxSizeBytes);
        } else {
            resp.body = ep.body;
        }

        return Result<FetchResponse>::ok(std::move(resp));
    }

    return Result<FetchResponse>::error(StatusCode::MAX_REDIRECTS_EXCEEDED, 
        "Maximum redirects exceeded (" + std::to_string(req.maxRedirects) + ")");
}

size_t FakeFetcher::fetchCount(const std::string& url) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto norm = UrlNormalizer::normalize(url);
    std::string key = norm.isOk() ? norm.value() : url;
    auto it = m_fetchCounts.find(key);
    return (it != m_fetchCounts.end()) ? it->second : 0;
}

size_t FakeFetcher::totalFetches() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalFetches;
}

void FakeFetcher::resetStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fetchCounts.clear();
    m_totalFetches = 0;
}

} // namespace crawler

