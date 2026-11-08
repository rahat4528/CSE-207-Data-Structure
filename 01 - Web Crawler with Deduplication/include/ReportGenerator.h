#ifndef CRAWLER_REPORT_GENERATOR_H
#define CRAWLER_REPORT_GENERATOR_H

#include "Common.h"
#include "StatsDashboard.h"
#include "CrawlGraph.h"
#include "PageStorage.h"
#include <string>
#include <vector>

namespace crawler {

struct BrokenLink {
    std::string targetUrl;
    int statusCode{0};
    std::vector<std::string> referringUrls;
};

struct DuplicateCluster {
    uint64_t representativeHash{0};
    bool isExact{false};
    std::vector<std::string> urls;
};

struct AuthorityPage {
    std::string url;
    size_t inDegree{0};
    size_t outDegree{0};
    uint32_t depth{0};
    std::string title;
};

class ReportGenerator {
public:
    static std::vector<BrokenLink> findBrokenLinks(const CrawlGraph& graph, const PageStorage& storage);
    static std::vector<DuplicateCluster> findNearDuplicateClusters(const PageStorage& storage, uint32_t maxDistance = 3);
    static std::vector<AuthorityPage> getTopAuthorityPages(const CrawlGraph& graph, const PageStorage& storage, size_t topN = 10);

    static std::string generateMarkdownReport(const CrawlStats& stats,
                                              const CrawlGraph& graph,
                                              const PageStorage& storage);

    static bool exportMarkdownReport(const std::string& filepath,
                                     const CrawlStats& stats,
                                     const CrawlGraph& graph,
                                     const PageStorage& storage);
};

} // namespace crawler

#endif // CRAWLER_REPORT_GENERATOR_H

