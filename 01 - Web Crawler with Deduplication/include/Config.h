#ifndef CRAWLER_CONFIG_H
#define CRAWLER_CONFIG_H

#include "Common.h"
#include <string>
#include <vector>
#include <cstdint>

namespace crawler {

enum class DomainMode {
    ANY = 0,
    SAME_HOST,
    SAME_DOMAIN
};

inline std::string_view domainModeToString(DomainMode mode) noexcept {
    switch (mode) {
        case DomainMode::SAME_HOST: return "SAME_HOST";
        case DomainMode::SAME_DOMAIN: return "SAME_DOMAIN";
        default: return "ANY";
    }
}

inline std::ostream& operator<<(std::ostream& os, DomainMode mode) {
    return os << domainModeToString(mode);
}


inline Result<DomainMode> domainModeFromString(std::string_view sv) {
    if (sv == "any" || sv == "ANY") return Result<DomainMode>::ok(DomainMode::ANY);
    if (sv == "same_host" || sv == "SAME_HOST") return Result<DomainMode>::ok(DomainMode::SAME_HOST);
    if (sv == "same_domain" || sv == "SAME_DOMAIN") return Result<DomainMode>::ok(DomainMode::SAME_DOMAIN);
    return Result<DomainMode>::error(StatusCode::INVALID_URL, "Unknown domain mode: " + std::string(sv));
}

struct Config {
    std::vector<std::string> seedUrls;
    uint32_t maxDepth{3};
    uint32_t maxPages{100};
    uint32_t concurrency{4};
    std::string userAgent{"SearchBot/1.0 (+https://github.com/rahat4528/CSE-207-Data-Structure)"};

    uint32_t perHostDelayMs{1000};
    uint32_t connectTimeoutMs{3000};
    uint32_t totalTimeoutMs{5000};
    uint32_t maxRedirects{10};
    size_t maxResponseSizeBytes{5 * 1024 * 1024}; // 5 MiB

    // Bloom filter sizing
    size_t bloomCapacity{100000};
    double bloomTargetFpr{0.01};

    // Domain & robot policy
    DomainMode domainMode{DomainMode::ANY};
    std::vector<std::string> allowedDomains;
    bool respectRobots{true};
    bool enableSimHash{true};

    // Safety
    bool blockPrivateIps{true};
    bool allowTestLoopback{false}; // strictly for offline unit tests if enabled

    // Persistence & Export
    std::string checkpointPath{"data/crawler.state"};
    uint32_t checkpointIntervalSec{30};
    std::string exportJsonPath{"data/crawled_pages.json"};
    std::string exportCsvPath{"data/crawled_pages.csv"};
    std::string exportDotPath{"data/crawl_graph.dot"};
    std::string exportHtmlPath{"data/crawl_graph.html"};
    std::string exportReportPath{"data/crawl_report.md"};
    bool runDemo{false};

    Result<void> validate() const;

    static Result<Config> parseCommandLine(int argc, char* argv[]);
};

} // namespace crawler

#endif // CRAWLER_CONFIG_H
