#include "Url.h"
#include <algorithm>
#include <cctype>

namespace crawler {

namespace {

std::string toLowerAscii(std::string_view sv) {
    std::string out;
    out.reserve(sv.size());
    for (char c : sv) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

} // namespace

Result<Url> Url::parse(std::string_view rawUrl) {
    if (rawUrl.empty()) {
        return Result<Url>::error(StatusCode::INVALID_URL, "Empty URL string");
    }
    if (rawUrl.size() > MAX_URL_LENGTH) {
        return Result<Url>::error(StatusCode::INVALID_URL, "URL exceeds maximum permitted length");
    }

    Url url;

    // Check for scheme separator
    size_t schemeEnd = rawUrl.find("://");
    if (schemeEnd == std::string_view::npos) {
        // Check if starts with other schemes like javascript:, mailto:, tel:, file:, data:
        size_t colonPos = rawUrl.find(':');
        if (colonPos != std::string_view::npos) {
            std::string rejectedScheme = toLowerAscii(rawUrl.substr(0, colonPos));
            return Result<Url>::error(StatusCode::UNSUPPORTED_SCHEME, 
                "Unsupported URL scheme: " + rejectedScheme);
        }
        return Result<Url>::error(StatusCode::INVALID_URL, "Missing scheme separator (://)");
    }

    std::string schemeStr = toLowerAscii(rawUrl.substr(0, schemeEnd));
    if (schemeStr != "http" && schemeStr != "https") {
        return Result<Url>::error(StatusCode::UNSUPPORTED_SCHEME, "Scheme must be http or https, got: " + schemeStr);
    }
    url.m_scheme = std::move(schemeStr);

    std::string_view rest = rawUrl.substr(schemeEnd + 3);
    if (rest.empty()) {
        return Result<Url>::error(StatusCode::INVALID_URL, "Missing host authority");
    }

    // Split authority from path/query/fragment
    size_t pathStart = rest.find_first_of("/?#");
    std::string_view authority = (pathStart == std::string_view::npos) ? rest : rest.substr(0, pathStart);
    std::string_view pathAndBeyond = (pathStart == std::string_view::npos) ? "" : rest.substr(pathStart);

    // Authority may contain userinfo (username:password@) - reject or strip
    size_t atPos = authority.find('@');
    if (atPos != std::string_view::npos) {
        authority = authority.substr(atPos + 1);
    }

    if (authority.empty()) {
        return Result<Url>::error(StatusCode::INVALID_URL, "Host authority is empty");
    }

    // Host & Port
    std::string hostStr;
    uint16_t portNum = (url.m_scheme == "https") ? 443 : 80;

    if (authority.front() == '[') {
        // IPv6 literal
        size_t closeBracket = authority.find(']');
        if (closeBracket == std::string_view::npos) {
            return Result<Url>::error(StatusCode::INVALID_URL, "Unclosed IPv6 bracket in host");
        }
        hostStr = toLowerAscii(authority.substr(0, closeBracket + 1));
        std::string_view afterBracket = authority.substr(closeBracket + 1);
        if (!afterBracket.empty()) {
            if (afterBracket.front() != ':') {
                return Result<Url>::error(StatusCode::INVALID_URL, "Malformed port after IPv6 host");
            }
            try {
                unsigned long p = std::stoul(std::string(afterBracket.substr(1)));
                if (p == 0 || p > 65535) return Result<Url>::error(StatusCode::INVALID_URL, "Port out of range");
                portNum = static_cast<uint16_t>(p);
            } catch (...) {
                return Result<Url>::error(StatusCode::INVALID_URL, "Invalid port number");
            }
        }
    } else {
        // Standard IPv4 or hostname
        size_t colonPos = authority.find(':');
        if (colonPos != std::string_view::npos) {
            hostStr = toLowerAscii(authority.substr(0, colonPos));
            std::string_view portStr = authority.substr(colonPos + 1);
            if (portStr.empty()) {
                return Result<Url>::error(StatusCode::INVALID_URL, "Empty port in authority");
            }
            try {
                unsigned long p = std::stoul(std::string(portStr));
                if (p == 0 || p > 65535) return Result<Url>::error(StatusCode::INVALID_URL, "Port out of range");
                portNum = static_cast<uint16_t>(p);
            } catch (...) {
                return Result<Url>::error(StatusCode::INVALID_URL, "Invalid port number");
            }
        } else {
            hostStr = toLowerAscii(authority);
        }
    }

    // Validate host characters
    for (char c : hostStr) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\\' || c == '<' || c == '>') {
            return Result<Url>::error(StatusCode::INVALID_URL, "Host contains invalid characters");
        }
    }

    url.m_host = std::move(hostStr);
    url.m_port = portNum;

    // Path, query, fragment
    std::string_view pathPart = pathAndBeyond;
    std::string_view queryPart;
    std::string_view fragPart;

    size_t hashPos = pathPart.find('#');
    if (hashPos != std::string_view::npos) {
        fragPart = pathPart.substr(hashPos + 1);
        pathPart = pathPart.substr(0, hashPos);
    }

    size_t qPos = pathPart.find('?');
    if (qPos != std::string_view::npos) {
        queryPart = pathPart.substr(qPos + 1);
        pathPart = pathPart.substr(0, qPos);
    }

    url.m_path = pathPart.empty() ? "/" : std::string(pathPart);
    url.m_query = std::string(queryPart);
    url.m_fragment = std::string(fragPart);
    url.m_isValid = true;

    return Result<Url>::ok(std::move(url));
}

std::string Url::origin() const {
    std::string res = m_scheme + "://" + m_host;
    bool isDefaultPort = (m_scheme == "http" && m_port == 80) || (m_scheme == "https" && m_port == 443);
    if (!isDefaultPort && m_port != 0) {
        res += ":" + std::to_string(m_port);
    }
    return res;
}

std::string Url::toString() const {
    std::string res = origin();
    if (!m_path.empty()) {
        if (m_path.front() != '/') res.push_back('/');
        res += m_path;
    } else {
        res.push_back('/');
    }
    if (!m_query.empty()) {
        res.push_back('?');
        res += m_query;
    }
    if (!m_fragment.empty()) {
        res.push_back('#');
        res += m_fragment;
    }
    return res;
}

} // namespace crawler

