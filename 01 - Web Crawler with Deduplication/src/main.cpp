#include "Common.h"
#include "Config.h"
#include "CrawlerEngine.h"
#include "FakeFetcher.h"
#include "HttpFetcher.h"
#include "WorkerPool.h"
#include "StatsDashboard.h"
#include "ReportGenerator.h"
#include "GraphVisualizer.h"
#include "Exporter.h"
#include "CheckpointManager.h"
#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace crawler;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --seed <url>              Add seed URL to crawl (can be repeated)\n"
              << "  --depth <n>               Maximum crawl depth (default: 3)\n"
              << "  --pages <n>               Maximum pages to crawl (default: 100)\n"
              << "  --concurrency <n>         Worker thread count (default: 4)\n"
              << "  --delay <ms>              Politeness delay per host in ms (default: 1000)\n"
              << "  --domain-mode <mode>      ANY | SAME_HOST | SAME_DOMAIN (default: ANY)\n"
              << "  --bloom-capacity <n>      Expected unique items for Bloom filter (default: 100000)\n"
              << "  --bloom-fpr <p>           Target false positive probability (default: 0.01)\n"
              << "  --user-agent <str>        Custom HTTP User-Agent header\n"
              << "  --no-robots               Disable robots.txt parsing and enforcement\n"
              << "  --no-simhash              Disable 64-bit SimHash near-duplicate detection\n"
              << "  --checkpoint <path>       State file path for atomic checkpoint save/resume\n"
              << "  --export-json <path>      Export crawled pages to JSON (default: data/crawled_pages.json)\n"
              << "  --export-csv <path>       Export crawled pages to CSV (default: data/crawled_pages.csv)\n"
              << "  --export-dot <path>       Export link graph to Graphviz DOT (default: data/crawl_graph.dot)\n"
              << "  --export-html <path>      Export interactive HTML viewer (default: data/crawl_graph.html)\n"
              << "  --export-report <path>    Export Markdown audit report (default: data/crawl_report.md)\n"
              << "  --demo                    Execute built-in deterministic simulation\n"
              << "  --help, -h                Show this help message and exit\n";
}

void setupSimulation(std::shared_ptr<FakeFetcher> fetcher) {
    // Root page
    fetcher->registerResponse("http://demo.search-engine.internal/", 200,
        "<!DOCTYPE html><html><head><title>Search Engine Demo Portal</title></head><body>"
        "<h1>Search Portal</h1>"
        "<p>Welcome to the search engine demonstration foundation.</p>"
        "<a href=\"/docs\">Documentation</a>"
        "<a href=\"/news\">Tech News</a>"
        "<a href=\"/products\">Products Catalog</a>"
        "<a href=\"/mirror-1\">Mirror Article 1</a>"
        "<a href=\"/dead-link\">Broken Link</a>"
        "<a href=\"https://external.example.com/\" rel=\"nofollow\">External Partner</a>"
        "</body></html>");

    // Documentation
    fetcher->registerResponse("http://demo.search-engine.internal/docs", 200,
        "<!DOCTYPE html><html><head><title>System Documentation</title></head><body>"
        "<h1>Documentation</h1>"
        "<p>Modern search engines crawl the web level by level with Level-Synchronous BFS. "
        "A Bloom filter provides rapid approximate duplicate checks to avoid revisiting pages. "
        "The exact visited set guarantees that no valid webpage is erroneously skipped during indexing.</p>"
        "<a href=\"/\">Home</a>"
        "<a href=\"/docs/architecture\">Architecture Overview</a>"
        "<a href=\"/docs/bloom-filter\">Bloom Filter Details</a>"
        "</body></html>");

    // Architecture
    fetcher->registerResponse("http://demo.search-engine.internal/docs/architecture", 200,
        "<!DOCTYPE html><html><head><title>Crawler Architecture</title></head><body>"
        "<h1>Crawler Architecture</h1>"
        "<p>The engine separates URL frontier, deduplication, fetching, parsing, and link graph maintenance.</p>"
        "<a href=\"/docs\">Back to Docs</a>"
        "</body></html>");

    // Bloom Filter Details
    fetcher->registerResponse("http://demo.search-engine.internal/docs/bloom-filter", 200,
        "<!DOCTYPE html><html><head><title>Bloom Filter Optimization</title></head><body>"
        "<h1>Bloom Filter Mathematics</h1>"
        "<p>Kirsch-Mitzenmacher optimization allows k hash values to be simulated using two 64-bit independent hashes: "
        "gi(x) = h1(x) + i * h2(x) mod m.</p>"
        "<a href=\"/docs\">Back to Docs</a>"
        "</body></html>");

    // Tech News
    fetcher->registerResponse("http://demo.search-engine.internal/news", 200,
        "<!DOCTYPE html><html><head><title>Latest Technology News</title></head><body>"
        "<h1>Tech News</h1>"
        "<p>Advances in distributed systems and algorithmic data compression enable hyper-scale crawling.</p>"
        "<a href=\"/\">Home</a>"
        "<a href=\"/mirror-2\">Syndicated News Story</a>"
        "</body></html>");

    // Near-duplicate stories (SimHash test)
    fetcher->registerResponse("http://demo.search-engine.internal/mirror-1", 200,
        "<!DOCTYPE html><html><head><title>Breaking AI Discovery</title></head><body>"
        "<p>The high performance search platform indexes billions of documents across web servers worldwide daily. "
        "Automated deduplication prevents storage bloat and saves server network bandwidth. "
        "All visited links are safely tracked in synchronized memory structures.</p>"
        "<a href=\"/news\">News</a>"
        "</body></html>");

    fetcher->registerResponse("http://demo.search-engine.internal/mirror-2", 200,
        "<!DOCTYPE html><html><head><title>Breaking AI Discovery Syndicate</title></head><body>"
        "<p>The high performance search platform indexes millions of documents across web servers worldwide daily. "
        "Automated deduplication prevents storage bloat and saves server network bandwidth. "
        "All visited links are safely tracked in synchronized memory structures.</p>"
        "<a href=\"/news\">News</a>"
        "</body></html>");

    // Products
    fetcher->registerResponse("http://demo.search-engine.internal/products", 200,
        "<!DOCTYPE html><html><head><title>Our Products</title></head><body>"
        "<h1>Products</h1>"
        "<p>Explore our fast data structure libraries and storage engines.</p>"
        "<a href=\"/\">Home</a>"
        "<a href=\"/products/crawler\">Web Crawler v1.0</a>"
        "</body></html>");

    fetcher->registerResponse("http://demo.search-engine.internal/products/crawler", 200,
        "<!DOCTYPE html><html><head><title>Web Crawler Engine</title></head><body>"
        "<h1>Web Crawler v1.0</h1>"
        "<p>Pure ISO C++20 search engine crawling foundation.</p>"
        "<a href=\"/products\">Back to Products</a>"
        "</body></html>");

    // 404 Broken link
    fetcher->registerResponse("http://demo.search-engine.internal/dead-link", 404,
        "<!DOCTYPE html><html><head><title>Not Found</title></head><body>"
        "<h1>404 Not Found</h1><p>Requested resource does not exist.</p>"
        "</body></html>");
}

int main(int argc, char* argv[]) {
    std::cout << "\033[1m\033[36m"
              << "============================================================\n"
              << "  WEB CRAWLER WITH DEDUPLICATION (ISO C++20)\n"
              << "  Search Engine Foundation - Two-Stage Deduplication Engine\n"
              << "============================================================\n\033[0m\n";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    auto parseRes = Config::parseCommandLine(argc, argv);
    if (parseRes.isError()) {
        std::cerr << "[ERROR] " << parseRes.errorMessage() << "\n\n";
        printUsage(argv[0]);
        return 1;
    }

    Config config = parseRes.value();

    if (config.seedUrls.empty() && !config.runDemo) {
        std::cout << "No seed URLs provided. Defaulting to built-in simulation (--demo).\n";
        config.runDemo = true;
    }

    std::filesystem::create_directories("data");

    std::shared_ptr<IFetcher> fetcher;
    if (config.runDemo) {
        std::cout << "[INFO] Initializing in-memory deterministic simulation graph...\n";
        auto fake = std::make_shared<FakeFetcher>();
        setupSimulation(fake);
        fetcher = fake;
        config.seedUrls = {"http://demo.search-engine.internal/"};
        config.maxDepth = 3;
        config.maxPages = 50;
    } else {
        std::cout << "[INFO] Initializing live socket HTTP fetcher...\n";
        fetcher = std::make_shared<HttpFetcher>(config.allowTestLoopback);
    }

    CrawlerEngine engine(config, fetcher);
    engine.init();

    std::cout << "[INFO] Starting Level-Synchronous BFS crawl traversal...\n";

    size_t lastCrawled = 0;
    while (engine.step()) {
        size_t currentCrawled = engine.crawledCount();
        if (currentCrawled != lastCrawled && (currentCrawled % 2 == 0 || engine.queueSize() == 0)) {
            lastCrawled = currentCrawled;
            CrawlStats stats = StatsDashboard::collect(engine);
            StatsDashboard::printLive(stats, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

    // Final dashboard render
    CrawlStats finalStats = StatsDashboard::collect(engine);
    StatsDashboard::printLive(finalStats, true);

    std::cout << "\n[INFO] Crawl completed successfully.\n";

    // 1. Checkpoint save
    std::cout << "[INFO] Writing state checkpoint to: " << config.checkpointPath << " ... ";
    if (CheckpointManager::save(config.checkpointPath, engine).isOk()) {
        std::cout << "SUCCESS (CRC32 Verified)\n";
    } else {
        std::cout << "FAILED\n";
    }

    // 2. Export JSON
    std::cout << "[INFO] Exporting crawled pages to JSON: " << config.exportJsonPath << " ... ";
    if (Exporter::exportJson(config.exportJsonPath, engine).isOk()) {
        std::cout << "SUCCESS\n";
    } else {
        std::cout << "FAILED\n";
    }

    // 3. Export CSV
    std::cout << "[INFO] Exporting crawled pages to CSV: " << config.exportCsvPath << " ... ";
    if (Exporter::exportCsv(config.exportCsvPath, engine).isOk()) {
        std::cout << "SUCCESS\n";
    } else {
        std::cout << "FAILED\n";
    }

    // 4. Export DOT
    std::cout << "[INFO] Exporting link graph to Graphviz DOT: " << config.exportDotPath << " ... ";
    if (GraphVisualizer::exportDotFile(config.exportDotPath, engine.graph(), engine.storage())) {
        std::cout << "SUCCESS\n";
    } else {
        std::cout << "FAILED\n";
    }

    // 5. Export HTML Viewer
    std::cout << "[INFO] Exporting interactive HTML viewer: " << config.exportHtmlPath << " ... ";
    if (GraphVisualizer::exportHtmlViewerFile(config.exportHtmlPath, engine.graph(), engine.storage(), "Search Engine Link Graph")) {
        std::cout << "SUCCESS\n";
    } else {
        std::cout << "FAILED\n";
    }

    // 6. Export Markdown Audit Report
    std::cout << "[INFO] Exporting Markdown audit report: " << config.exportReportPath << " ... ";
    if (ReportGenerator::exportMarkdownReport(config.exportReportPath, finalStats, engine.graph(), engine.storage())) {
        std::cout << "SUCCESS\n";
    } else {
        std::cout << "FAILED\n";
    }

    std::cout << "\n[DONE] All crawler operations, deduplication metrics, and reports generated.\n";
    return 0;
}

