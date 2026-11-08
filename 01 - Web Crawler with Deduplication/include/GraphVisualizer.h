#ifndef CRAWLER_GRAPH_VISUALIZER_H
#define CRAWLER_GRAPH_VISUALIZER_H

#include "Common.h"
#include "CrawlGraph.h"
#include "PageStorage.h"
#include <string>

namespace crawler {

class GraphVisualizer {
public:
    static std::string toDot(const CrawlGraph& graph, const PageStorage& storage);
    static std::string toMermaid(const CrawlGraph& graph, size_t maxNodes = 60);
    static std::string toHtmlViewer(const CrawlGraph& graph,
                                   const PageStorage& storage,
                                   const std::string& title = "Web Crawler Link Graph");

    static bool exportDotFile(const std::string& filepath,
                             const CrawlGraph& graph,
                             const PageStorage& storage);

    static bool exportMermaidFile(const std::string& filepath,
                                 const CrawlGraph& graph,
                                 size_t maxNodes = 60);

    static bool exportHtmlViewerFile(const std::string& filepath,
                                     const CrawlGraph& graph,
                                     const PageStorage& storage,
                                     const std::string& title = "Web Crawler Link Graph");
};

} // namespace crawler

#endif // CRAWLER_GRAPH_VISUALIZER_H

