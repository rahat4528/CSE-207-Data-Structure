#ifndef CRAWLER_CRAWLER_ENGINE_H
#define CRAWLER_CRAWLER_ENGINE_H

#include "Common.h"
#include "Config.h"
#include "IFetcher.h"
#include "BloomFilter.h"
#include "ExactVisitedSet.h"
#include "BfsScheduler.h"
#include "CrawlGraph.h"
#include "PageStorage.h"
#include <memory>
#include <atomic>
#include <chrono>

namespace crawler {

class CrawlerEngine {
public:
    CrawlerEngine(Config config, std::shared_ptr<IFetcher> fetcher);

    void init();
    bool step();
    void run();

    const Config& config() const noexcept { return m_config; }
    const BloomFilter& bloomFilter() const noexcept { return m_bloom; }
    const ExactVisitedSet& visitedSet() const noexcept { return m_visited; }
    const BfsScheduler& scheduler() const noexcept { return m_scheduler; }
    const CrawlGraph& graph() const noexcept { return m_graph; }
    const PageStorage& storage() const noexcept { return m_storage; }

    size_t crawledCount() const noexcept { return m_scheduler.totalCrawled(); }
    size_t queueSize() const noexcept { return m_scheduler.queueSize(); }
    size_t falsePositiveDetections() const noexcept { return m_bloomFalsePositives; }

    size_t candidateUrlsSeen() const noexcept { return m_candidateUrlsSeen; }
    size_t bloomNegatives() const noexcept { return m_bloomNegatives; }
    size_t bloomPositives() const noexcept { return m_bloomPositives; }
    size_t exactDuplicates() const noexcept { return m_exactDuplicates; }
    size_t totalBytesFetched() const noexcept { return m_totalBytesFetched; }
    double elapsedSeconds() const noexcept;

private:
    Config m_config;
    std::shared_ptr<IFetcher> m_fetcher;
    BloomFilter m_bloom;
    ExactVisitedSet m_visited;
    BfsScheduler m_scheduler;
    CrawlGraph m_graph;
    PageStorage m_storage;

    std::atomic<size_t> m_bloomFalsePositives{0};
    std::atomic<size_t> m_candidateUrlsSeen{0};
    std::atomic<size_t> m_bloomNegatives{0};
    std::atomic<size_t> m_bloomPositives{0};
    std::atomic<size_t> m_exactDuplicates{0};
    std::atomic<size_t> m_totalBytesFetched{0};

    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_endTime;
    bool m_hasStarted{false};
    bool m_hasFinished{false};
};

} // namespace crawler

#endif // CRAWLER_CRAWLER_ENGINE_H

