#ifndef CRAWLER_ROBOTS_PARSER_H
#define CRAWLER_ROBOTS_PARSER_H

#include "Common.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <shared_mutex>
#include <mutex>

namespace crawler {

struct RobotsRule {
    std::string pattern;
    bool allow{true}; // true = Allow, false = Disallow
};

class RobotsPolicy {
public:
    RobotsPolicy() = default;

    static RobotsPolicy parse(std::string_view robotsContent, std::string_view userAgent);

    bool isAllowed(std::string_view path) const;
    std::optional<uint32_t> crawlDelaySec() const noexcept { return m_crawlDelay; }

private:
    std::vector<RobotsRule> m_rules;
    std::optional<uint32_t> m_crawlDelay;

    static bool matchPattern(std::string_view pattern, std::string_view path);
};

class RobotsCache {
public:
    RobotsCache() = default;

    void setPolicy(const std::string& host, RobotsPolicy policy);
    std::optional<RobotsPolicy> getPolicy(const std::string& host) const;

    bool isAllowed(const std::string& host, const std::string& path) const;

    void clear();

private:
    std::unordered_map<std::string, RobotsPolicy> m_cache;
    mutable std::shared_mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_ROBOTS_PARSER_H

