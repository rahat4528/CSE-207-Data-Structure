#ifndef CRAWLER_URL_NORMALIZER_H
#define CRAWLER_URL_NORMALIZER_H

#include "Common.h"
#include "Url.h"
#include <string>
#include <string_view>
#include <vector>

namespace crawler {

class UrlNormalizer {
public:
    static Result<std::string> normalize(std::string_view rawUrl);
    static Result<std::string> resolve(std::string_view baseUrl, std::string_view relativeUrl);
    static Result<std::string> resolveWithBaseHref(std::string_view pageUrl, 
                                                  std::string_view baseHref, 
                                                  std::string_view relativeUrl);

    // Algorithmic sub-components exposed for testing
    static std::string removeDotSegments(std::string_view path);
    static std::string normalizePercentEncoding(std::string_view str);
    static std::string normalizeQueryString(std::string_view query);

private:
    static bool isUnreservedChar(unsigned char c) noexcept;
    static int hexVal(char c) noexcept;
};

} // namespace crawler

#endif // CRAWLER_URL_NORMALIZER_H

