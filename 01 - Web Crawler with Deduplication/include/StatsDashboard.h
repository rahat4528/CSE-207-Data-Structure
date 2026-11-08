#ifndef CRAWLER_STATS_DASHBOARD_H
#define CRAWLER_STATS_DASHBOARD_H

#include "Common.h"
#include "CrawlerEngine.h"
#include <string>
#include <map>
#include <cstdint>

namespace crawler {

struct CrawlStats {
    size_t pagesCrawled{0};
    size_t queueSize{0};
    uint32_t currentDepth{0};
    double elapsedSeconds{0.0};
    double pagesPerSecond{0.0};
    size_t totalBytesDownloaded{0};

    std::map<int, size_t> statusCounts;
    std::map<uint32_t, size_t> depthHistogram;

    size_t candidateUrlsTested{0};
    size_t bloomNegatives{0};
    size_t bloomPositives{0};
    size_t exactDuplicates{0};
    size_t bloomFalsePositives{0};
    double empiricalFpr{0.0};
    double theoreticalFpr{0.0};
    size_t bloomBitsSet{0};
    size_t bloomTotalBits{0};
    double bloomFillRatio{0.0};

    size_t nearDuplicateCount{0};
    size_t estimatedMemoryBytes{0};
};

class StatsDashboard {
public:
    static CrawlStats collect(const CrawlerEngine& engine);
    static std::string renderAscii(const CrawlStats& stats, bool useAnsiColors = true);
    static void printLive(const CrawlStats& stats, bool clearScreen = false);

private:
    static std::string formatBytes(size_t bytes);
    static std::string progressBar(double fraction, int width = 24);
};

} // namespace crawler

#endif // CRAWLER_STATS_DASHBOARD_H

