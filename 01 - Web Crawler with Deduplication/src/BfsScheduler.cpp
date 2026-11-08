#include "BfsScheduler.h"

namespace crawler {

BfsScheduler::BfsScheduler(uint32_t maxDepth, uint32_t maxPages)
    : m_maxDepth(maxDepth), m_maxPages(maxPages) {}

void BfsScheduler::seed(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentLevelQueue.push({url, 0});
}

bool BfsScheduler::enqueueDiscovered(const std::string& url, uint32_t sourceDepth) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t nextDepth = sourceDepth + 1;
    if (nextDepth > m_maxDepth) {
        return false; // Beyond max depth limit
    }
    m_nextLevelQueue.push({url, nextDepth});
    return true;
}

std::optional<CrawlTask> BfsScheduler::nextTask() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Stop if page limit reached
    if (m_totalCrawled >= m_maxPages) {
        return std::nullopt;
    }

    if (m_currentLevelQueue.empty()) {
        if (m_nextLevelQueue.empty()) {
            return std::nullopt; // Finished
        }
        // Advance to next depth level
        m_currentLevelQueue = std::move(m_nextLevelQueue);
        m_nextLevelQueue = std::queue<CrawlTask>{};
        if (!m_currentLevelQueue.empty()) {
            m_currentDepth = m_currentLevelQueue.front().depth;
        }
    }

    CrawlTask task = m_currentLevelQueue.front();
    m_currentLevelQueue.pop();
    return task;
}

void BfsScheduler::markCrawled() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_totalCrawled++;
}

bool BfsScheduler::isFinished() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_totalCrawled >= m_maxPages) return true;
    return m_currentLevelQueue.empty() && m_nextLevelQueue.empty();
}

size_t BfsScheduler::queueSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentLevelQueue.size() + m_nextLevelQueue.size();
}

uint32_t BfsScheduler::currentDepth() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentDepth;
}

size_t BfsScheduler::totalCrawled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalCrawled;
}

void BfsScheduler::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentLevelQueue = std::queue<CrawlTask>{};
    m_nextLevelQueue = std::queue<CrawlTask>{};
    m_currentDepth = 0;
    m_totalCrawled = 0;
}

} // namespace crawler

