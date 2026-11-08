#ifndef CRAWLER_HTML_PARSER_H
#define CRAWLER_HTML_PARSER_H

#include "Common.h"
#include <string>
#include <string_view>
#include <vector>

namespace crawler {

struct ExtractedLink {
    std::string targetUrl;
    std::string anchorText;
    bool nofollow{false};
};

struct ParsedDocument {
    std::string title;
    std::string metaDescription;
    std::string baseHref;
    std::string canonicalUrl;
    bool metaNoindex{false};
    bool metaNofollow{false};
    std::vector<ExtractedLink> links;
    std::string cleanText;
};

class HtmlParser {
public:
    static ParsedDocument parse(std::string_view pageUrl, std::string_view html);

    static std::string decodeEntities(std::string_view text);
    static std::string collapseWhitespace(std::string_view text);

private:
    static std::string toLowerAscii(std::string_view sv);
    static std::string getAttributeValue(std::string_view tagContent, std::string_view attrName);
};

} // namespace crawler

#endif // CRAWLER_HTML_PARSER_H

