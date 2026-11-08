#ifndef CRAWLER_URL_H
#define CRAWLER_URL_H

#include "Common.h"
#include <string>
#include <string_view>
#include <cstdint>
#include <optional>

namespace crawler {

class Url {
public:
    Url() = default;

    static Result<Url> parse(std::string_view rawUrl);

    bool isValid() const noexcept { return m_isValid; }
    const std::string& scheme() const noexcept { return m_scheme; }
    const std::string& host() const noexcept { return m_host; }
    uint16_t port() const noexcept { return m_port; }
    const std::string& path() const noexcept { return m_path; }
    const std::string& query() const noexcept { return m_query; }
    const std::string& fragment() const noexcept { return m_fragment; }

    bool isHttps() const noexcept { return m_scheme == "https"; }
    bool isHttp() const noexcept { return m_scheme == "http"; }

    std::string toString() const;
    std::string origin() const; // scheme://host[:port]

    // Setters for normalization
    void setScheme(std::string s) { m_scheme = std::move(s); }
    void setHost(std::string h) { m_host = std::move(h); }
    void setPort(uint16_t p) { m_port = p; }
    void setPath(std::string p) { m_path = std::move(p); }
    void setQuery(std::string q) { m_query = std::move(q); }
    void setFragment(std::string f) { m_fragment = std::move(f); }

    static constexpr size_t MAX_URL_LENGTH = 2048;

private:
    bool m_isValid{false};
    std::string m_scheme;
    std::string m_host;
    uint16_t m_port{0};
    std::string m_path{"/"};
    std::string m_query;
    std::string m_fragment;
};

} // namespace crawler

#endif // CRAWLER_URL_H

