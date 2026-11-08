#include "SsrfGuard.h"
#include <sstream>
#include <cctype>

namespace crawler {

bool SsrfGuard::parseIpv4(std::string_view ipStr, uint32_t& outIp) noexcept {
    unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
    char d1 = 0, d2 = 0, d3 = 0;
    std::string s(ipStr);
    std::istringstream iss(s);

    if ((iss >> b0 >> d1 >> b1 >> d2 >> b2 >> d3 >> b3) && 
        d1 == '.' && d2 == '.' && d3 == '.' &&
        b0 <= 255 && b1 <= 255 && b2 <= 255 && b3 <= 255) {
        std::string trailing;
        if (!(iss >> trailing)) {
            outIp = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
            return true;
        }
    }
    return false;
}

bool SsrfGuard::isBlockedIpv4(uint32_t ip, bool allowLoopback) noexcept {
    uint8_t b0 = static_cast<uint8_t>((ip >> 24) & 0xFF);
    uint8_t b1 = static_cast<uint8_t>((ip >> 16) & 0xFF);

    // 0.0.0.0/8 (Current network)
    if (b0 == 0) return true;

    // 127.0.0.0/8 (Loopback)
    if (b0 == 127) {
        return !allowLoopback;
    }

    // 10.0.0.0/8 (Private-Use)
    if (b0 == 10) return true;

    // 172.16.0.0/12 (Private-Use: 172.16.0.0 - 172.31.255.255)
    if (b0 == 172 && (b1 >= 16 && b1 <= 31)) return true;

    // 192.168.0.0/16 (Private-Use)
    if (b0 == 192 && b1 == 168) return true;

    // 169.254.0.0/16 (Link-Local & Cloud Metadata 169.254.169.254)
    if (b0 == 169 && b1 == 254) return true;

    // 100.64.0.0/10 (Carrier-Grade NAT: 100.64.0.0 - 100.127.255.255)
    if (b0 == 100 && (b1 >= 64 && b1 <= 127)) return true;

    // 224.0.0.0/4 (Multicast)
    if (b0 >= 224 && b0 <= 239) return true;

    // 240.0.0.0/4 (Reserved / Future Use)
    if (b0 >= 240) return true;

    return false;
}

bool SsrfGuard::isBlockedIp(std::string_view ipStr, bool allowLoopback) {
    // Check IPv4
    uint32_t ipv4 = 0;
    if (parseIpv4(ipStr, ipv4)) {
        return isBlockedIpv4(ipv4, allowLoopback);
    }

    // Check IPv6
    std::string s(ipStr);
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    // IPv6 Loopback (::1 or [::1])
    if (s == "::1" || s == "[::1]") {
        return !allowLoopback;
    }

    // IPv6 Unspecified (::)
    if (s == "::" || s == "[::]") {
        return true;
    }

    // IPv4-mapped IPv6 (::ffff:127.0.0.1, ::ffff:10.0.0.1, etc.)
    const std::string mappedPrefix = "::ffff:";
    if (s.rfind(mappedPrefix, 0) == 0) {
        std::string_view embeddedV4 = std::string_view(s).substr(mappedPrefix.size());
        uint32_t embIp = 0;
        if (parseIpv4(embeddedV4, embIp)) {
            return isBlockedIpv4(embIp, allowLoopback);
        }
    }

    // Unique-local (fc00::/7) -> starts with fc or fd
    if (s.rfind("fc", 0) == 0 || s.rfind("fd", 0) == 0 ||
        s.rfind("[fc", 0) == 0 || s.rfind("[fd", 0) == 0) {
        return true;
    }

    // Link-local (fe80::/10) -> starts with fe8, fe9, fea, feb
    if (s.rfind("fe8", 0) == 0 || s.rfind("fe9", 0) == 0 || 
        s.rfind("fea", 0) == 0 || s.rfind("feb", 0) == 0 ||
        s.rfind("[fe8", 0) == 0 || s.rfind("[fe9", 0) == 0) {
        return true;
    }

    return false;
}

Result<void> SsrfGuard::validateIp(std::string_view ipStr, bool allowLoopback) {
    if (isBlockedIp(ipStr, allowLoopback)) {
        return Result<void>::error(StatusCode::SSRF_BLOCKED, 
            "SSRF Blocked: IP address belongs to private/internal range: " + std::string(ipStr));
    }
    return Result<void>::ok();
}

} // namespace crawler

