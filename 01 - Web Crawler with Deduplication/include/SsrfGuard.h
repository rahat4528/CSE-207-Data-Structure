#ifndef CRAWLER_SSRF_GUARD_H
#define CRAWLER_SSRF_GUARD_H

#include "Common.h"
#include <string>
#include <string_view>
#include <vector>

namespace crawler {

class SsrfGuard {
public:
    static bool isBlockedIpv4(uint32_t ipBigEndian, bool allowLoopback = false) noexcept;
    static bool isBlockedIp(std::string_view ipStr, bool allowLoopback = false);

    // Parses string IP (IPv4 or IPv6) and verifies if it is safe
    static Result<void> validateIp(std::string_view ipStr, bool allowLoopback = false);

    // Parse dotted-quad IPv4 into 32-bit big-endian integer
    static bool parseIpv4(std::string_view ipStr, uint32_t& outIp) noexcept;
};

} // namespace crawler

#endif // CRAWLER_SSRF_GUARD_H

