#ifndef CRAWLER_EXPORTER_H
#define CRAWLER_EXPORTER_H

#include "Common.h"
#include "CrawlerEngine.h"
#include <string>
#include <string_view>

namespace crawler {

class Exporter {
public:
    static Result<void> exportJson(const std::string& filePath, const CrawlerEngine& engine);
    static Result<void> exportCsv(const std::string& filePath, const CrawlerEngine& engine);

    static std::string escapeJsonString(std::string_view str);
    static std::string escapeCsvCell(std::string_view cell);
};

} // namespace crawler

#endif // CRAWLER_EXPORTER_H

