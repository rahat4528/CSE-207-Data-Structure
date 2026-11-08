#ifndef CRAWLER_IFETCHER_H
#define CRAWLER_IFETCHER_H

#include "Common.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <cstdint>

namespace crawler {

struct FetchRequest {
    std::string url;
    std::string method{"GET"};
    std::unordered_map<std::string, std::string> headers;
    uint32_t connectTimeoutMs{3000};
    uint32_t totalTimeoutMs{5000};
    uint32_t maxRedirects{10};
    size_t maxSizeBytes{5 * 1024 * 1024}; // 5 MiB
};

struct FetchResponse {
    int statusCode{0};
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string finalUrl;
    std::vector<std::string> redirectChain;
    uint32_t responseTimeMs{0};
    std::optional<uint32_t> retryAfterSec;
    std::string contentType;

    bool isSuccess() const noexcept {
        return statusCode >= 200 && statusCode < 300;
    }

    bool isRedirect() const noexcept {
        return statusCode >= 300 && statusCode < 400;
    }

    bool isClientError() const noexcept {
        return statusCode >= 400 && statusCode < 500;
    }

    bool isServerError() const noexcept {
        return statusCode >= 500 && statusCode < 600;
    }

    bool isTooManyRequests() const noexcept {
        return statusCode == 429;
    }

    std::optional<std::string> getHeader(std::string_view name) const {
        for (const auto& [k, v] : headers) {
            if (k.size() == name.size()) {
                bool match = true;
                for (size_t i = 0; i < k.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(k[i])) !=
                        std::tolower(static_cast<unsigned char>(name[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match) return v;
            }
        }
        return std::nullopt;
    }
};

class IFetcher {
public:
    virtual ~IFetcher() = default;
    virtual Result<FetchResponse> fetch(const FetchRequest& req) = 0;
};

} // namespace crawler

#endif // CRAWLER_IFETCHER_H

