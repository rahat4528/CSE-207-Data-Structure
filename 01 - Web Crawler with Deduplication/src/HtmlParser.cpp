#include "HtmlParser.h"
#include "UrlNormalizer.h"
#include <algorithm>
#include <cctype>

namespace crawler {

std::string HtmlParser::toLowerAscii(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) {
        s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return s;
}

std::string HtmlParser::decodeEntities(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            size_t semi = text.find(';', i + 1);
            if (semi != std::string_view::npos && semi - i <= 10) {
                std::string_view ent = text.substr(i, semi - i + 1);
                if (ent == "&amp;") { out.push_back('&'); i = semi; continue; }
                if (ent == "&lt;") { out.push_back('<'); i = semi; continue; }
                if (ent == "&gt;") { out.push_back('>'); i = semi; continue; }
                if (ent == "&quot;") { out.push_back('"'); i = semi; continue; }
                if (ent == "&#39;" || ent == "&apos;") { out.push_back('\''); i = semi; continue; }
                if (ent == "&nbsp;") { out.push_back(' '); i = semi; continue; }
            }
        }
        out.push_back(text[i]);
    }
    return out;
}

std::string HtmlParser::collapseWhitespace(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    bool inSpace = false;

    for (char c : text) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (!inSpace) {
                out.push_back(' ');
                inSpace = true;
            }
        } else {
            out.push_back(c);
            inSpace = false;
        }
    }

    // Trim leading and trailing whitespace
    while (!out.empty() && out.front() == ' ') out.erase(0, 1);
    while (!out.empty() && out.back() == ' ') out.pop_back();

    return out;
}

std::string HtmlParser::getAttributeValue(std::string_view tagContent, std::string_view attrName) {
    std::string lowerTag = toLowerAscii(tagContent);
    std::string lowerAttr = toLowerAscii(attrName);

    size_t pos = 0;
    while ((pos = lowerTag.find(lowerAttr, pos)) != std::string::npos) {
        // Ensure it's a boundary before the attribute name
        if (pos > 0 && !std::isspace(static_cast<unsigned char>(lowerTag[pos - 1]))) {
            pos += lowerAttr.size();
            continue;
        }

        size_t afterAttr = pos + lowerAttr.size();
        // Skip whitespace after attr name
        while (afterAttr < lowerTag.size() && std::isspace(static_cast<unsigned char>(lowerTag[afterAttr]))) {
            afterAttr++;
        }

        if (afterAttr < lowerTag.size() && lowerTag[afterAttr] == '=') {
            size_t valStart = afterAttr + 1;
            while (valStart < lowerTag.size() && std::isspace(static_cast<unsigned char>(lowerTag[valStart]))) {
                valStart++;
            }

            if (valStart < tagContent.size()) {
                char quote = tagContent[valStart];
                if (quote == '"' || quote == '\'') {
                    size_t valEnd = tagContent.find(quote, valStart + 1);
                    if (valEnd != std::string_view::npos) {
                        return decodeEntities(tagContent.substr(valStart + 1, valEnd - valStart - 1));
                    }
                } else {
                    // Unquoted attribute
                    size_t valEnd = tagContent.find_first_of(" \t\r\n>", valStart);
                    if (valEnd == std::string_view::npos) valEnd = tagContent.size();
                    return decodeEntities(tagContent.substr(valStart, valEnd - valStart));
                }
            }
        }
        pos += lowerAttr.size();
    }
    return "";
}

ParsedDocument HtmlParser::parse(std::string_view pageUrl, std::string_view html) {
    ParsedDocument doc;
    std::string rawTextAccumulator;

    size_t i = 0;
    const size_t len = html.size();

    bool inScript = false;
    bool inStyle = false;

    while (i < len) {
        if (html[i] == '<') {
            // Check for comment <!-- ... -->
            if (i + 3 < len && html.substr(i, 4) == "<!--") {
                size_t commentEnd = html.find("-->", i + 4);
                if (commentEnd == std::string_view::npos) break;
                i = commentEnd + 3;
                continue;
            }

            size_t tagEnd = html.find('>', i);
            if (tagEnd == std::string_view::npos) {
                // Unclosed malformed tag: consume rest
                break;
            }

            std::string_view fullTag = html.substr(i + 1, tagEnd - i - 1);
            std::string lowerTag = toLowerAscii(fullTag);

            // Handle closing script/style
            if (lowerTag.rfind("/script", 0) == 0) { inScript = false; i = tagEnd + 1; continue; }
            if (lowerTag.rfind("/style", 0) == 0)  { inStyle = false;  i = tagEnd + 1; continue; }

            if (inScript || inStyle) {
                i = tagEnd + 1;
                continue;
            }

            if (lowerTag.rfind("script", 0) == 0) { inScript = true; i = tagEnd + 1; continue; }
            if (lowerTag.rfind("style", 0) == 0)  { inStyle = true;  i = tagEnd + 1; continue; }

            // Title tag: <title>...</title>
            if (lowerTag.rfind("title", 0) == 0) {
                size_t closeTitle = html.find("</", tagEnd + 1);
                if (closeTitle != std::string_view::npos) {
                    size_t afterClose = html.find('>', closeTitle);
                    if (afterClose != std::string_view::npos) {
                        std::string_view titleContent = html.substr(tagEnd + 1, closeTitle - tagEnd - 1);
                        doc.title = collapseWhitespace(decodeEntities(titleContent));
                        i = afterClose + 1;
                        continue;
                    }
                }
            }

            // Base href: <base href="...">
            if (lowerTag.rfind("base", 0) == 0) {
                std::string href = getAttributeValue(fullTag, "href");
                if (!href.empty()) {
                    doc.baseHref = href;
                }
            }

            // Meta tags: description, robots
            if (lowerTag.rfind("meta", 0) == 0) {
                std::string name = getAttributeValue(fullTag, "name");
                std::string content = getAttributeValue(fullTag, "content");
                std::string lowerName = toLowerAscii(name);
                std::string lowerContent = toLowerAscii(content);

                if (lowerName == "description") {
                    doc.metaDescription = content;
                } else if (lowerName == "robots") {
                    if (lowerContent.find("noindex") != std::string::npos) {
                        doc.metaNoindex = true;
                    }
                    if (lowerContent.find("nofollow") != std::string::npos) {
                        doc.metaNofollow = true;
                    }
                }
            }

            // Canonical link: <link rel="canonical" href="...">
            if (lowerTag.rfind("link", 0) == 0) {
                std::string rel = toLowerAscii(getAttributeValue(fullTag, "rel"));
                if (rel == "canonical") {
                    std::string href = getAttributeValue(fullTag, "href");
                    if (!href.empty()) {
                        auto normCanonical = UrlNormalizer::resolveWithBaseHref(pageUrl, doc.baseHref, href);
                        if (normCanonical.isOk()) {
                            doc.canonicalUrl = normCanonical.value();
                        }
                    }
                }
            }

            // Anchor tag: <a href="..." rel="...">
            if (lowerTag.rfind("a", 0) == 0 && (lowerTag.size() == 1 || std::isspace(static_cast<unsigned char>(lowerTag[1])))) {
                std::string href = getAttributeValue(fullTag, "href");
                std::string rel = toLowerAscii(getAttributeValue(fullTag, "rel"));
                bool nofollow = doc.metaNofollow || (rel.find("nofollow") != std::string::npos);

                if (!href.empty()) {
                    auto resolved = UrlNormalizer::resolveWithBaseHref(pageUrl, doc.baseHref, href);
                    if (resolved.isOk()) {
                        ExtractedLink link;
                        link.targetUrl = resolved.value();
                        link.nofollow = nofollow;
                        doc.links.push_back(std::move(link));
                    }
                }
            }

            i = tagEnd + 1;
        } else {
            if (!inScript && !inStyle) {
                rawTextAccumulator.push_back(html[i]);
            }
            i++;
        }
    }

    if (doc.metaNofollow) {
        for (auto& link : doc.links) {
            link.nofollow = true;
        }
    }

    doc.cleanText = collapseWhitespace(decodeEntities(rawTextAccumulator));
    return doc;
}


} // namespace crawler
