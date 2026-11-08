#ifndef CRAWLER_BFS_SCHEDULER_H
#define CRAWLER_BFS_SCHEDULER_H

#include "Common.h"
#include <string>
#include <queue>
#include <optional>
#include <cstdint>
#include <mutex>

namespace crawler {

struct CrawlTask {
    std::string url;
    uint32_t depth{0};
};

class BfsScheduler {
public:
    BfsScheduler(uint32_t maxDepth, uint32_t maxPages);

    void seed(const std::string& url);
    bool enqueueDiscovered(const std::string& url, uint32_t sourceDepth);

    std::optional<CrawlTask> nextTask();

    bool isFinished() const;
    size_t queueSize() const;
    uint32_t currentDepth() const;
    size_t totalCrawled() const;

    void markCrawled();

    // Reset or load for resume
    void clear();

private:
    uint32_t m_maxDepth{3};
    uint32_t m_maxPages{100};
    uint32_t m_currentDepth{0};
    size_t m_totalCrawled{0};

    std::queue<CrawlTask> m_currentLevelQueue;
    std::queue<CrawlTask> m_nextLevelQueue;
    mutable std::mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_BFS_SCHEDULER_H

