#include "PageStorage.h"

namespace crawler {

void PageStorage::storePage(CrawledPage page) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_pages[page.url] = std::move(page);
}

std::optional<CrawledPage> PageStorage::getPage(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_pages.find(url);
    if (it != m_pages.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool PageStorage::hasPage(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_pages.contains(url);
}

size_t PageStorage::pageCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_pages.size();
}

std::vector<CrawledPage> PageStorage::getAllPages() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<CrawledPage> result;
    result.reserve(m_pages.size());
    for (const auto& [url, page] : m_pages) {
        result.push_back(page);
    }
    return result;
}

void PageStorage::clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_pages.clear();
}

} // namespace crawler

