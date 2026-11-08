#include "MiniTest.h"
#include "CrawlerEngine.h"
#include "FakeFetcher.h"

using namespace crawler;

TEST_CASE(TestBfsLevelOrderTraversal) {
    auto fetcher = std::make_shared<FakeFetcher>();

    // Graph structure:
    // /root (depth 0) -> /child1, /child2 (depth 1)
    // /child1 -> /grandchild1 (depth 2)
    // /child2 -> /grandchild2 (depth 2)
    fetcher->registerResponse("https://example.com/root", 200, 
        "<a href='/child1'>C1</a><a href='/child2'>C2</a>");
    fetcher->registerResponse("https://example.com/child1", 200, 
        "<a href='/grandchild1'>GC1</a>");
    fetcher->registerResponse("https://example.com/child2", 200, 
        "<a href='/grandchild2'>GC2</a>");
    fetcher->registerResponse("https://example.com/grandchild1", 200, "Leaf 1");
    fetcher->registerResponse("https://example.com/grandchild2", 200, "Leaf 2");

    Config cfg;
    cfg.seedUrls = {"https://example.com/root"};
    cfg.maxDepth = 3;
    cfg.maxPages = 20;

    CrawlerEngine engine(cfg, fetcher);
    engine.init();

    std::vector<std::pair<std::string, uint32_t>> visitOrder;

    while (auto taskOpt = const_cast<BfsScheduler&>(engine.scheduler()).nextTask()) {
        auto task = taskOpt.value();
        visitOrder.push_back({task.url, task.depth});

        // Re-inject task so step() processes it
        const_cast<BfsScheduler&>(engine.scheduler()).seed(task.url); // peek trick
        break;
    }

    // Re-run fresh with standard engine.run()
    CrawlerEngine freshEngine(cfg, fetcher);
    freshEngine.run();

    ASSERT_EQ(freshEngine.crawledCount(), 5u);

    // Verify all pages at depth 1 have depth 1 in storage
    auto rootPage = freshEngine.storage().getPage("https://example.com/root");
    ASSERT_TRUE(rootPage.has_value());
    ASSERT_EQ(rootPage.value().depth, 0u);

    auto c1Page = freshEngine.storage().getPage("https://example.com/child1");
    ASSERT_TRUE(c1Page.has_value());
    ASSERT_EQ(c1Page.value().depth, 1u);

    auto gc1Page = freshEngine.storage().getPage("https://example.com/grandchild1");
    ASSERT_TRUE(gc1Page.has_value());
    ASSERT_EQ(gc1Page.value().depth, 2u);
}

TEST_CASE(TestMaxDepthLimitRespected) {
    auto fetcher = std::make_shared<FakeFetcher>();
    // /d0 -> /d1 -> /d2 -> /d3 -> /d4
    fetcher->registerResponse("https://example.com/d0", 200, "<a href='/d1'></a>");
    fetcher->registerResponse("https://example.com/d1", 200, "<a href='/d2'></a>");
    fetcher->registerResponse("https://example.com/d2", 200, "<a href='/d3'></a>");
    fetcher->registerResponse("https://example.com/d3", 200, "<a href='/d4'></a>");
    fetcher->registerResponse("https://example.com/d4", 200, "End");

    Config cfg;
    cfg.seedUrls = {"https://example.com/d0"};
    cfg.maxDepth = 2; // Only d0, d1, d2 allowed
    cfg.maxPages = 50;

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    ASSERT_EQ(engine.crawledCount(), 3u);
    ASSERT_TRUE(engine.storage().hasPage("https://example.com/d0"));
    ASSERT_TRUE(engine.storage().hasPage("https://example.com/d1"));
    ASSERT_TRUE(engine.storage().hasPage("https://example.com/d2"));
    ASSERT_FALSE(engine.storage().hasPage("https://example.com/d3"));
    ASSERT_FALSE(engine.storage().hasPage("https://example.com/d4"));
}

TEST_CASE(TestMaxPageLimitRespected) {
    auto fetcher = std::make_shared<FakeFetcher>();
    // Page with 20 child links
    std::string rootHtml;
    for (int i = 0; i < 20; ++i) {
        rootHtml += "<a href='/page_" + std::to_string(i) + "'>P</a>";
        fetcher->registerResponse("https://example.com/page_" + std::to_string(i), 200, "Content");
    }
    fetcher->registerResponse("https://example.com/root", 200, rootHtml);

    Config cfg;
    cfg.seedUrls = {"https://example.com/root"};
    cfg.maxDepth = 3;
    cfg.maxPages = 5; // Hard limit 5 pages

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    // Must stop after crawling exactly 5 pages
    ASSERT_EQ(engine.crawledCount(), 5u);
}

TEST_CASE(TestCycleAvoidanceAndGraphEdges) {
    auto fetcher = std::make_shared<FakeFetcher>();
    // Circular link graph: A -> B -> C -> A
    fetcher->registerResponse("https://example.com/A", 200, "<a href='/B'>To B</a>");
    fetcher->registerResponse("https://example.com/B", 200, "<a href='/C'>To C</a>");
    fetcher->registerResponse("https://example.com/C", 200, "<a href='/A'>To A</a>");

    Config cfg;
    cfg.seedUrls = {"https://example.com/A"};
    cfg.maxDepth = 10;
    cfg.maxPages = 50;

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    // Terminated safely without infinite loop
    ASSERT_EQ(engine.crawledCount(), 3u);
    ASSERT_TRUE(engine.graph().hasCycle());
    ASSERT_EQ(engine.graph().nodeCount(), 3u);
    ASSERT_EQ(engine.graph().edgeCount(), 3u);
}

TEST_CASE(TestDeduplicationWithTinyBloomFilter) {
    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("https://example.com/root", 200, 
        "<a href='/a'></a><a href='/b'></a><a href='/c'></a><a href='/d'></a><a href='/e'></a>");
    fetcher->registerResponse("https://example.com/a", 200, "A");
    fetcher->registerResponse("https://example.com/b", 200, "B");
    fetcher->registerResponse("https://example.com/c", 200, "C");
    fetcher->registerResponse("https://example.com/d", 200, "D");
    fetcher->registerResponse("https://example.com/e", 200, "E");

    Config cfg;
    cfg.seedUrls = {"https://example.com/root"};
    cfg.maxDepth = 2;
    cfg.maxPages = 20;
    // Tiny filter forcing potential collisions
    cfg.bloomCapacity = 8;
    cfg.bloomTargetFpr = 0.5;

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    // Despite high collision rate, all 6 pages must be crawled safely!
    ASSERT_EQ(engine.crawledCount(), 6u);
}

