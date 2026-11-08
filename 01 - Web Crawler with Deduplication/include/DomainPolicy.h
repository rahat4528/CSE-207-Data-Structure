#ifndef CRAWLER_DOMAIN_POLICY_H
#define CRAWLER_DOMAIN_POLICY_H

#include "Common.h"
#include "Config.h"
#include "Url.h"
#include <string>
#include <vector>
#include <string_view>

namespace crawler {

class DomainPolicy {
public:
    explicit DomainPolicy(DomainMode mode = DomainMode::ANY);

    void setMode(DomainMode mode) noexcept { m_mode = mode; }
    void addAllowedHost(std::string host);
    void addAllowedDomain(std::string domain);

    void addIncludePattern(std::string pattern);
    void addExcludePattern(std::string pattern);

    bool isAllowed(const Url& url) const;
    bool isAllowed(std::string_view rawUrl) const;

private:
    DomainMode m_mode{DomainMode::ANY};
    std::vector<std::string> m_allowedHosts;
    std::vector<std::string> m_allowedDomains;
    std::vector<std::string> m_includePatterns;
    std::vector<std::string> m_excludePatterns;

    static bool matchPattern(std::string_view pattern, std::string_view text);
};

} // namespace crawler

#endif // CRAWLER_DOMAIN_POLICY_H

