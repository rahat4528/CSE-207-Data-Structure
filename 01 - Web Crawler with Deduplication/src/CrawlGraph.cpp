#include "CrawlGraph.h"
#include <algorithm>
#include <sstream>

namespace crawler {

void CrawlGraph::addNode(const std::string& url) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nodes.insert(url);
    if (!m_forwardAdj.contains(url)) {
        m_forwardAdj[url] = {};
    }
    if (!m_reverseAdj.contains(url)) {
        m_reverseAdj[url] = {};
    }
}

void CrawlGraph::addEdge(const std::string& fromUrl, const std::string& toUrl) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nodes.insert(fromUrl);
    m_nodes.insert(toUrl);

    auto& outList = m_forwardAdj[fromUrl];
    if (std::find(outList.begin(), outList.end(), toUrl) == outList.end()) {
        outList.push_back(toUrl);
        m_reverseAdj[toUrl].push_back(fromUrl);
        m_edgeCount++;
    }
}

size_t CrawlGraph::inDegree(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_reverseAdj.find(url);
    return (it != m_reverseAdj.end()) ? it->second.size() : 0;
}

size_t CrawlGraph::outDegree(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_forwardAdj.find(url);
    return (it != m_forwardAdj.end()) ? it->second.size() : 0;
}

std::vector<std::string> CrawlGraph::getOutEdges(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_forwardAdj.find(url);
    return (it != m_forwardAdj.end()) ? it->second : std::vector<std::string>{};
}

std::vector<std::string> CrawlGraph::getInEdges(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_reverseAdj.find(url);
    return (it != m_reverseAdj.end()) ? it->second : std::vector<std::string>{};
}

size_t CrawlGraph::nodeCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_nodes.size();
}

size_t CrawlGraph::edgeCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_edgeCount;
}

std::vector<std::string> CrawlGraph::getOrphanPages() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<std::string> orphans;
    for (const auto& node : m_nodes) {
        auto it = m_reverseAdj.find(node);
        if (it == m_reverseAdj.end() || it->second.empty()) {
            orphans.push_back(node);
        }
    }
    return orphans;
}

std::vector<std::string> CrawlGraph::getDeadEndPages() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<std::string> deadEnds;
    for (const auto& node : m_nodes) {
        auto it = m_forwardAdj.find(node);
        if (it == m_forwardAdj.end() || it->second.empty()) {
            deadEnds.push_back(node);
        }
    }
    return deadEnds;
}

std::vector<std::pair<std::string, size_t>> CrawlGraph::getMostLinkedPages(size_t topK) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<std::pair<std::string, size_t>> rankings;
    rankings.reserve(m_nodes.size());

    for (const auto& node : m_nodes) {
        auto it = m_reverseAdj.find(node);
        size_t deg = (it != m_reverseAdj.end()) ? it->second.size() : 0;
        rankings.push_back({node, deg});
    }

    std::sort(rankings.begin(), rankings.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    if (rankings.size() > topK) {
        rankings.resize(topK);
    }
    return rankings;
}

bool CrawlGraph::dfsCycle(const std::string& node,
                         std::unordered_map<std::string, int>& color) const {
    color[node] = 1; // GRAY (currently in recursion stack)

    auto it = m_forwardAdj.find(node);
    if (it != m_forwardAdj.end()) {
        for (const auto& neighbor : it->second) {
            int c = color[neighbor];
            if (c == 1) {
                return true; // Cycle detected
            }
            if (c == 0 && dfsCycle(neighbor, color)) {
                return true;
            }
        }
    }

    color[node] = 2; // BLACK (visited and finished)
    return false;
}

bool CrawlGraph::hasCycle() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::unordered_map<std::string, int> color; // 0=WHITE, 1=GRAY, 2=BLACK
    for (const auto& node : m_nodes) {
        color[node] = 0;
    }

    for (const auto& node : m_nodes) {
        if (color[node] == 0) {
            if (dfsCycle(node, color)) {
                return true;
            }
        }
    }
    return false;
}

std::string CrawlGraph::toDot() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::ostringstream oss;
    oss << "digraph CrawlGraph {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [shape=box, style=rounded, fontname=\"Arial\"];\n";

    for (const auto& [src, targets] : m_forwardAdj) {
        for (const auto& dst : targets) {
            oss << "  \"" << src << "\" -> \"" << dst << "\";\n";
        }
    }
    oss << "}\n";
    return oss.str();
}

} // namespace crawler

