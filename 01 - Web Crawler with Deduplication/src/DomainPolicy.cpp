#include "DomainPolicy.h"
#include <algorithm>
#include <cctype>

namespace crawler {

namespace {

std::string toLowerAscii(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return s;
}

} // namespace

DomainPolicy::DomainPolicy(DomainMode mode) : m_mode(mode) {}

void DomainPolicy::addAllowedHost(std::string host) {
    m_allowedHosts.push_back(toLowerAscii(host));
}

void DomainPolicy::addAllowedDomain(std::string domain) {
    m_allowedDomains.push_back(toLowerAscii(domain));
}

void DomainPolicy::addIncludePattern(std::string pattern) {
    m_includePatterns.push_back(std::move(pattern));
}

void DomainPolicy::addExcludePattern(std::string pattern) {
    m_excludePatterns.push_back(std::move(pattern));
}

bool DomainPolicy::matchPattern(std::string_view pattern, std::string_view text) {
    if (pattern.empty()) return true;
    // Prefix match or wildcard
    size_t starPos = pattern.find('*');
    if (starPos == std::string_view::npos) {
        return text.rfind(pattern, 0) == 0;
    }
    std::string_view prefix = pattern.substr(0, starPos);
    return text.rfind(prefix, 0) == 0;
}

bool DomainPolicy::isAllowed(const Url& url) const {
    if (!url.isValid()) return false;

    std::string host = toLowerAscii(url.host());

    if (m_mode == DomainMode::SAME_HOST) {
        bool match = false;
        for (const auto& allowed : m_allowedHosts) {
            if (host == allowed) {
                match = true;
                break;
            }
        }
        if (!match) return false;
    } else if (m_mode == DomainMode::SAME_DOMAIN) {
        bool match = false;
        for (const auto& allowed : m_allowedDomains) {
            if (host == allowed || (host.size() > allowed.size() && 
                host.ends_with("." + allowed))) {
                match = true;
                break;
            }
        }
        if (!match) return false;
    }

    std::string pathAndQuery = url.path();
    if (!url.query().empty()) {
        pathAndQuery += "?" + url.query();
    }

    // Check exclude patterns
    for (const auto& excl : m_excludePatterns) {
        if (matchPattern(excl, pathAndQuery)) {
            return false;
        }
    }

    // Check include patterns
    if (!m_includePatterns.empty()) {
        bool matchedInclude = false;
        for (const auto& incl : m_includePatterns) {
            if (matchPattern(incl, pathAndQuery)) {
                matchedInclude = true;
                break;
            }
        }
        if (!matchedInclude) return false;
    }

    return true;
}

bool DomainPolicy::isAllowed(std::string_view rawUrl) const {
    auto parsed = Url::parse(rawUrl);
    if (parsed.isError()) return false;
    return isAllowed(parsed.value());
}

} // namespace crawler

