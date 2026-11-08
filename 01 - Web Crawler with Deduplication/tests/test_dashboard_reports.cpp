#include "MiniTest.h"
#include "CrawlerEngine.h"
#include "FakeFetcher.h"
#include "ContentHasher.h"
#include "StatsDashboard.h"
#include "ReportGenerator.h"
#include "GraphVisualizer.h"
#include <filesystem>
#include <fstream>

using namespace crawler;

TEST_CASE(TestStatsDashboardMetrics) {
    Config cfg;
    cfg.seedUrls = {"http://dashboard-test.com/"};
    cfg.maxDepth = 2;
    cfg.maxPages = 10;

    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("http://dashboard-test.com/", 200,
                             "<a href=\"/page1\">Page 1</a><a href=\"/page404\">Missing</a>");
    fetcher->registerResponse("http://dashboard-test.com/page1", 200,
                             "<title>Page 1 Title</title><p>Welcome to page 1</p><a href=\"/\">Home</a>");
    fetcher->registerResponse("http://dashboard-test.com/page404", 404,
                             "<html>Not found</html>");

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    CrawlStats stats = StatsDashboard::collect(engine);

    ASSERT_EQ(stats.pagesCrawled, 3ULL);
    ASSERT_EQ(stats.queueSize, 0ULL);
    ASSERT_TRUE(stats.statusCounts[200] >= 2ULL);
    ASSERT_TRUE(stats.statusCounts[404] >= 1ULL);
    ASSERT_TRUE(stats.depthHistogram[0] == 1ULL);
    ASSERT_TRUE(stats.bloomBitsSet > 0);
    ASSERT_TRUE(stats.estimatedMemoryBytes > 0);

    std::string ascii = StatsDashboard::renderAscii(stats, false);
    ASSERT_TRUE(ascii.find("WEB CRAWLER") != std::string::npos);
    ASSERT_TRUE(ascii.find("Pages Crawled:") != std::string::npos);
    ASSERT_TRUE(ascii.find("Two-Stage Deduplication Engine:") != std::string::npos);
}

TEST_CASE(TestBrokenLinksDetection) {
    Config cfg;
    cfg.seedUrls = {"http://broken-test.com/"};
    cfg.maxDepth = 2;

    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("http://broken-test.com/", 200,
                             "<a href=\"/broken-page\">Dead Link</a>");
    fetcher->registerResponse("http://broken-test.com/broken-page", 404,
                             "Page does not exist");

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    auto broken = ReportGenerator::findBrokenLinks(engine.graph(), engine.storage());
    ASSERT_EQ(broken.size(), 1ULL);
    ASSERT_EQ(broken[0].targetUrl, "http://broken-test.com/broken-page");
    ASSERT_EQ(broken[0].statusCode, 404);
    ASSERT_EQ(broken[0].referringUrls.size(), 1ULL);
    ASSERT_EQ(broken[0].referringUrls[0], "http://broken-test.com/");
}

TEST_CASE(TestNearDuplicateClusters) {
    PageStorage storage;

    CrawledPage p1;
    p1.url = "http://dupe.com/original";
    p1.statusCode = 200;
    p1.cleanText = "Modern search engines use web crawlers to discover documents across the internet. "
                   "Crawlers follow hyperlinks level by level, maintaining a queue of discovered URLs. "
                   "A Bloom filter provides fast approximate duplicate checks to avoid revisiting pages. "
                   "The exact visited set guarantees that no valid webpage is erroneously skipped during indexing.";
    p1.contentHash = ContentHasher::computeExactHash(p1.cleanText);
    p1.simHash = ContentHasher::computeSimHash(p1.cleanText);
    storage.storePage(p1);

    CrawledPage p2;
    p2.url = "http://dupe.com/near-copy";
    p2.statusCode = 200;
    p2.cleanText = "Modern search engines use web crawlers to discover documents across the internet. "
                   "Crawlers follow hyperlinks level by level, maintaining a queue of discovered URLs. "
                   "A Bloom filter provides rapid approximate duplicate checks to avoid revisiting pages. "
                   "The exact visited set guarantees that no valid webpage is erroneously skipped during indexing.";
    p2.contentHash = ContentHasher::computeExactHash(p2.cleanText);
    p2.simHash = ContentHasher::computeSimHash(p2.cleanText);
    storage.storePage(p2);

    auto clusters = ReportGenerator::findNearDuplicateClusters(storage, 3);
    ASSERT_EQ(clusters.size(), 1ULL);
    ASSERT_EQ(clusters[0].urls.size(), 2ULL);
}

TEST_CASE(TestMarkdownReportAndGraphExports) {
    Config cfg;
    cfg.seedUrls = {"http://visual-test.com/"};
    cfg.maxDepth = 2;

    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("http://visual-test.com/", 200,
                             "<title>Root</title><a href=\"/a\">A</a><a href=\"/b\">B</a>");
    fetcher->registerResponse("http://visual-test.com/a", 200,
                             "<title>Page A</title><a href=\"/\">Home</a>");
    fetcher->registerResponse("http://visual-test.com/b", 200,
                             "<title>Page B</title><a href=\"/a\">A</a>");

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    CrawlStats stats = StatsDashboard::collect(engine);

    // Markdown Report
    std::string md = ReportGenerator::generateMarkdownReport(stats, engine.graph(), engine.storage());
    ASSERT_TRUE(md.find("# Web Crawler Audit & Performance Report") != std::string::npos);
    ASSERT_TRUE(md.find("Executive Summary") != std::string::npos);

    std::filesystem::create_directories("data/test_out");
    ASSERT_TRUE(ReportGenerator::exportMarkdownReport("data/test_out/report.md", stats, engine.graph(), engine.storage()));
    ASSERT_TRUE(std::filesystem::exists("data/test_out/report.md"));

    // DOT Visualization
    std::string dot = GraphVisualizer::toDot(engine.graph(), engine.storage());
    ASSERT_TRUE(dot.find("digraph CrawlGraph") != std::string::npos);
    ASSERT_TRUE(GraphVisualizer::exportDotFile("data/test_out/graph.dot", engine.graph(), engine.storage()));
    ASSERT_TRUE(std::filesystem::exists("data/test_out/graph.dot"));

    // Mermaid Visualization
    std::string mmd = GraphVisualizer::toMermaid(engine.graph());
    ASSERT_TRUE(mmd.find("flowchart TD") != std::string::npos);
    ASSERT_TRUE(GraphVisualizer::exportMermaidFile("data/test_out/graph.mmd", engine.graph()));
    ASSERT_TRUE(std::filesystem::exists("data/test_out/graph.mmd"));

    // HTML Interactive Viewer
    std::string html = GraphVisualizer::toHtmlViewer(engine.graph(), engine.storage(), "Test Viewer");
    ASSERT_TRUE(html.find("<!DOCTYPE html>") != std::string::npos);
    ASSERT_TRUE(html.find("canvas id=\"viewport\"") != std::string::npos);
    ASSERT_TRUE(GraphVisualizer::exportHtmlViewerFile("data/test_out/viewer.html", engine.graph(), engine.storage()));
    ASSERT_TRUE(std::filesystem::exists("data/test_out/viewer.html"));

    // Clean up test files
    std::filesystem::remove_all("data/test_out");
}

