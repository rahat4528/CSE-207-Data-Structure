#include "RobotsParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace crawler {

namespace {

std::string toLowerAscii(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return s;
}

std::string_view trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
    return sv;
}

} // namespace

bool RobotsPolicy::matchPattern(std::string_view pattern, std::string_view path) {
    if (pattern.empty()) return true;

    bool endAnchor = false;
    if (pattern.back() == '$') {
        endAnchor = true;
        pattern.remove_suffix(1);
    }

    size_t pIdx = 0;
    size_t sIdx = 0;
    size_t starIdx = std::string_view::npos;
    size_t matchIdx = 0;

    while (sIdx < path.size()) {
        if (pIdx == pattern.size() && !endAnchor) {
            return true; // Pattern prefix matched
        }
        if (pIdx < pattern.size() && (pattern[pIdx] == path[sIdx])) {
            pIdx++;
            sIdx++;
        } else if (pIdx < pattern.size() && pattern[pIdx] == '*') {
            starIdx = pIdx++;
            matchIdx = sIdx;
        } else if (starIdx != std::string_view::npos) {
            pIdx = starIdx + 1;
            sIdx = ++matchIdx;
        } else {
            return false;
        }
    }


    while (pIdx < pattern.size() && pattern[pIdx] == '*') {
        pIdx++;
    }

    if (endAnchor) {
        return pIdx == pattern.size() && sIdx == path.size();
    }
    return pIdx == pattern.size() || pIdx == 0;
}

RobotsPolicy RobotsPolicy::parse(std::string_view robotsContent, std::string_view userAgent) {
    RobotsPolicy policy;
    std::string lowerMyAgent = toLowerAscii(userAgent);

    std::vector<RobotsRule> specificRules;
    std::vector<RobotsRule> wildcardRules;
    std::optional<uint32_t> specificDelay;
    std::optional<uint32_t> wildcardDelay;

    bool inSpecificGroup = false;
    bool inWildcardGroup = false;

    std::istringstream stream{std::string(robotsContent)};
    std::string line;

    while (std::getline(stream, line)) {
        // Strip comments (#)
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos);
        }
        std::string_view trimmed = trim(line);
        if (trimmed.empty()) continue;

        size_t colon = trimmed.find(':');
        if (colon == std::string_view::npos) continue;

        std::string directive = toLowerAscii(trim(trimmed.substr(0, colon)));
        std::string_view value = trim(trimmed.substr(colon + 1));

        if (directive == "user-agent") {
            std::string agentVal = toLowerAscii(value);
            if (agentVal == "*") {
                inWildcardGroup = true;
                inSpecificGroup = false;
            } else if (!lowerMyAgent.empty() && lowerMyAgent.find(agentVal) != std::string::npos) {
                inSpecificGroup = true;
                inWildcardGroup = false;
            } else {
                inSpecificGroup = false;
                inWildcardGroup = false;
            }
        } else if (directive == "disallow") {
            if (value.empty()) {
                // Empty Disallow means "allow all"
                if (inSpecificGroup) specificRules.push_back({"/", true});
                else if (inWildcardGroup) wildcardRules.push_back({"/", true});
            } else {
                if (inSpecificGroup) specificRules.push_back({std::string(value), false});
                else if (inWildcardGroup) wildcardRules.push_back({std::string(value), false});
            }
        } else if (directive == "allow") {
            if (!value.empty()) {
                if (inSpecificGroup) specificRules.push_back({std::string(value), true});
                else if (inWildcardGroup) wildcardRules.push_back({std::string(value), true});
            }
        } else if (directive == "crawl-delay") {
            try {
                uint32_t d = static_cast<uint32_t>(std::stoul(std::string(value)));
                if (inSpecificGroup) specificDelay = d;
                else if (inWildcardGroup) wildcardDelay = d;
            } catch (...) {}
        }
    }

    if (!specificRules.empty()) {
        policy.m_rules = std::move(specificRules);
        policy.m_crawlDelay = specificDelay;
    } else {
        policy.m_rules = std::move(wildcardRules);
        policy.m_crawlDelay = wildcardDelay;
    }

    return policy;
}

bool RobotsPolicy::isAllowed(std::string_view path) const {
    if (m_rules.empty()) return true;

    const RobotsRule* bestMatch = nullptr;
    size_t longestMatchLen = 0;

    for (const auto& rule : m_rules) {
        if (matchPattern(rule.pattern, path)) {
            // RFC 9309: Longest match wins. If equal, Allow takes precedence.
            if (rule.pattern.size() > longestMatchLen) {
                longestMatchLen = rule.pattern.size();
                bestMatch = &rule;
            } else if (rule.pattern.size() == longestMatchLen && bestMatch && !bestMatch->allow && rule.allow) {
                bestMatch = &rule; // Allow wins tie
            }
        }
    }

    if (!bestMatch) {
        return true; // Default allowed
    }
    return bestMatch->allow;
}

void RobotsCache::setPolicy(const std::string& host, RobotsPolicy policy) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_cache[host] = std::move(policy);
}

std::optional<RobotsPolicy> RobotsCache::getPolicy(const std::string& host) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_cache.find(host);
    if (it != m_cache.end()) return it->second;
    return std::nullopt;
}

bool RobotsCache::isAllowed(const std::string& host, const std::string& path) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_cache.find(host);
    if (it != m_cache.end()) {
        return it->second.isAllowed(path);
    }
    return true; // Allowed by default if no cached robots.txt
}

void RobotsCache::clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_cache.clear();
}

} // namespace crawler
