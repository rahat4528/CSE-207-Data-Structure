#ifndef CRAWLER_CRAWL_GRAPH_H
#define CRAWLER_CRAWL_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <shared_mutex>
#include <mutex>

namespace crawler {

class CrawlGraph {
public:
    CrawlGraph() = default;

    void addNode(const std::string& url);
    void addEdge(const std::string& fromUrl, const std::string& toUrl);

    size_t inDegree(const std::string& url) const;
    size_t outDegree(const std::string& url) const;

    std::vector<std::string> getOutEdges(const std::string& url) const;
    std::vector<std::string> getInEdges(const std::string& url) const;

    size_t nodeCount() const;
    size_t edgeCount() const;

    std::vector<std::string> getOrphanPages() const;
    std::vector<std::string> getDeadEndPages() const;
    std::vector<std::pair<std::string, size_t>> getMostLinkedPages(size_t topK) const;

    bool hasCycle() const;

    std::string toDot() const;

private:
    std::unordered_set<std::string> m_nodes;
    std::unordered_map<std::string, std::vector<std::string>> m_forwardAdj;
    std::unordered_map<std::string, std::vector<std::string>> m_reverseAdj;
    size_t m_edgeCount{0};
    mutable std::shared_mutex m_mutex;

    bool dfsCycle(const std::string& node,
                  std::unordered_map<std::string, int>& color) const;
};

} // namespace crawler

#endif // CRAWLER_CRAWL_GRAPH_H

