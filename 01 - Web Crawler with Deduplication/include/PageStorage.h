#ifndef CRAWLER_PAGE_STORAGE_H
#define CRAWLER_PAGE_STORAGE_H

#include "Common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <optional>

namespace crawler {

struct CrawledPage {
    std::string url;
    int statusCode{200};
    uint32_t depth{0};
    std::string title;
    std::string metaDescription;
    std::string cleanText;
    size_t contentLength{0};
    uint64_t contentHash{0};
    uint64_t simHash{0};
    std::vector<std::string> outLinks;
    std::chrono::system_clock::time_point fetchTime{std::chrono::system_clock::now()};
};

class PageStorage {
public:
    PageStorage() = default;

    void storePage(CrawledPage page);
    std::optional<CrawledPage> getPage(const std::string& url) const;
    bool hasPage(const std::string& url) const;

    size_t pageCount() const;
    std::vector<CrawledPage> getAllPages() const;

    void clear();

private:
    std::unordered_map<std::string, CrawledPage> m_pages;
    mutable std::shared_mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_PAGE_STORAGE_H

