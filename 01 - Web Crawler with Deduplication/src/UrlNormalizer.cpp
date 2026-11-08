#include "UrlNormalizer.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace crawler {

namespace {

const char HEX_DIGITS[] = "0123456789ABCDEF";

bool isTrackingParam(std::string_view key) noexcept {
    return key == "utm_source" ||
           key == "utm_medium" ||
           key == "utm_campaign" ||
           key == "utm_term" ||
           key == "utm_content" ||
           key == "ref" ||
           key == "fbclid" ||
           key == "gclid" ||
           key == "msclkid";
}

} // namespace

bool UrlNormalizer::isUnreservedChar(unsigned char c) noexcept {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '.' || c == '_' || c == '~';
}

int UrlNormalizer::hexVal(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string UrlNormalizer::normalizePercentEncoding(std::string_view str) {
    std::string out;
    out.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int h1 = hexVal(str[i + 1]);
            int h2 = hexVal(str[i + 2]);
            if (h1 != -1 && h2 != -1) {
                unsigned char byteVal = static_cast<unsigned char>((h1 << 4) | h2);
                if (isUnreservedChar(byteVal)) {
                    out.push_back(static_cast<char>(byteVal));
                } else {
                    out.push_back('%');
                    out.push_back(HEX_DIGITS[h1]);
                    out.push_back(HEX_DIGITS[h2]);
                }
                i += 2;
                continue;
            }
        }
        out.push_back(str[i]);
    }
    return out;
}

std::string UrlNormalizer::removeDotSegments(std::string_view path) {
    std::string in(path);
    std::string out;

    while (!in.empty()) {
        // A: If input begins with "../" or "./"
        if (in.rfind("../", 0) == 0) {
            in.erase(0, 3);
        } else if (in.rfind("./", 0) == 0) {
            in.erase(0, 2);
        }
        // B: If input begins with "/./" or "/."
        else if (in.rfind("/./", 0) == 0) {
            in.erase(0, 2); // keeps leading '/'
        } else if (in == "/.") {
            in = "/";
        }
        // C: If input begins with "/../" or "/.."
        else if (in.rfind("/../", 0) == 0 || in == "/..") {
            if (in.rfind("/../", 0) == 0) {
                in.erase(0, 3); // keeps leading '/'
            } else {
                in = "/";
            }
            // Remove the last segment from out
            size_t lastSlash = out.rfind('/');
            if (lastSlash != std::string::npos) {
                out.erase(lastSlash);
            }
        }
        // D: If input is "." or ".."
        else if (in == "." || in == "..") {
            in.clear();
        }
        // E: Move the first path segment from input to output
        else {
            size_t segEnd = in.find('/', (in[0] == '/') ? 1 : 0);
            if (segEnd == std::string::npos) {
                out += in;
                in.clear();
            } else {
                out += in.substr(0, segEnd);
                in.erase(0, segEnd);
            }
        }
    }

    if (out.empty() || out[0] != '/') {
        out = "/" + out;
    }
    return out;
}

std::string UrlNormalizer::normalizeQueryString(std::string_view query) {
    if (query.empty()) return "";

    std::vector<std::string> params;
    size_t start = 0;
    while (start < query.size()) {
        size_t ampPos = query.find('&', start);
        std::string_view pair = (ampPos == std::string_view::npos) 
            ? query.substr(start) 
            : query.substr(start, ampPos - start);

        if (!pair.empty()) {
            size_t eqPos = pair.find('=');
            std::string_view key = (eqPos == std::string_view::npos) ? pair : pair.substr(0, eqPos);
            if (!isTrackingParam(key)) {
                params.push_back(std::string(pair));
            }
        }

        if (ampPos == std::string_view::npos) break;
        start = ampPos + 1;
    }

    if (params.empty()) return "";

    std::sort(params.begin(), params.end());

    std::string res;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) res.push_back('&');
        res += params[i];
    }
    return res;
}

Result<std::string> UrlNormalizer::normalize(std::string_view rawUrl) {
    auto parsed = Url::parse(rawUrl);
    if (parsed.isError()) {
        return Result<std::string>::error(parsed.code(), parsed.errorMessage());
    }

    Url url = std::move(parsed.value());

    // 1. Lowercase scheme & host are already handled by Url::parse
    // 2. Default port removal
    if ((url.scheme() == "http" && url.port() == 80) ||
        (url.scheme() == "https" && url.port() == 443)) {
        url.setPort(0);
    }

    // 3. Dot segment removal
    std::string cleanPath = removeDotSegments(url.path());

    // 4. Percent-encoding normalization on path
    cleanPath = normalizePercentEncoding(cleanPath);
    url.setPath(cleanPath);

    // 5. Query parameter sorting and tracking-param stripping
    if (!url.query().empty()) {
        std::string cleanQuery = normalizePercentEncoding(url.query());
        cleanQuery = normalizeQueryString(cleanQuery);
        url.setQuery(cleanQuery);
    }

    // 6. Fragment stripped (crawlers index document URLs, not in-page fragments)
    url.setFragment("");

    return Result<std::string>::ok(url.toString());
}

Result<std::string> UrlNormalizer::resolve(std::string_view baseUrl, std::string_view relativeUrl) {
    if (relativeUrl.empty()) {
        return normalize(baseUrl);
    }

    // If relativeUrl is already an absolute URL with scheme
    if (relativeUrl.find("://") != std::string_view::npos) {
        return normalize(relativeUrl);
    }

    // Check for rejected pseudo-schemes
    size_t colon = relativeUrl.find(':');
    if (colon != std::string_view::npos) {
        std::string schemePart(relativeUrl.substr(0, colon));
        for (char& c : schemePart) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (schemePart == "javascript" || schemePart == "mailto" || 
            schemePart == "tel" || schemePart == "data" || schemePart == "file") {
            return Result<std::string>::error(StatusCode::UNSUPPORTED_SCHEME, 
                "Unsupported scheme in link: " + schemePart);
        }
    }

    auto baseParsed = Url::parse(baseUrl);
    if (baseParsed.isError()) {
        return Result<std::string>::error(baseParsed.code(), baseParsed.errorMessage());
    }

    const Url& base = baseParsed.value();

    // Protocol-relative: //example.com/path
    if (relativeUrl.rfind("//", 0) == 0) {
        std::string full = base.scheme() + ":" + std::string(relativeUrl);
        return normalize(full);
    }

    // Root-relative: /path/to/page
    if (relativeUrl.front() == '/') {
        std::string full = base.origin() + std::string(relativeUrl);
        return normalize(full);
    }

    // Query-relative: ?key=value
    if (relativeUrl.front() == '?') {
        std::string full = base.origin() + base.path() + std::string(relativeUrl);
        return normalize(full);
    }

    // Fragment-only: #section -> keep base URL without fragment
    if (relativeUrl.front() == '#') {
        return normalize(baseUrl);
    }

    // Directory-relative: foo/bar.html
    std::string basePath = base.path();
    size_t lastSlash = basePath.rfind('/');
    std::string dir = (lastSlash == std::string::npos) ? "/" : basePath.substr(0, lastSlash + 1);

    std::string full = base.origin() + dir + std::string(relativeUrl);
    return normalize(full);
}

Result<std::string> UrlNormalizer::resolveWithBaseHref(std::string_view pageUrl, 
                                                      std::string_view baseHref, 
                                                      std::string_view relativeUrl) {
    if (baseHref.empty()) {
        return resolve(pageUrl, relativeUrl);
    }
    // First resolve baseHref against pageUrl
    auto effectiveBase = resolve(pageUrl, baseHref);
    if (effectiveBase.isError()) {
        return resolve(pageUrl, relativeUrl);
    }
    return resolve(effectiveBase.value(), relativeUrl);
}

} // namespace crawler

