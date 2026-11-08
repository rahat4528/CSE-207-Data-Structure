#include "MiniTest.h"
#include "WorkerPool.h"
#include "HttpFetcher.h"
#include "FakeFetcher.h"
#include "CrawlerEngine.h"
#include <atomic>
#include <set>

using namespace crawler;

TEST_CASE(TestWorkerPoolExecutionAndSynchronization) {
    WorkerPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 50; ++i) {
        pool.enqueue([&counter]() {
            counter++;
        });
    }

    pool.waitIdle();
    ASSERT_EQ(counter.load(), 50);
}

TEST_CASE(TestConcurrentCrawlDiscoversSamePageSet) {
    auto fetcher = std::make_shared<FakeFetcher>();

    // 10-page test web
    fetcher->registerResponse("https://example.com/start", 200, 
        "<a href='/p1'>1</a><a href='/p2'>2</a><a href='/p3'>3</a>");
    fetcher->registerResponse("https://example.com/p1", 200, "<a href='/p4'>4</a><a href='/p5'>5</a>");
    fetcher->registerResponse("https://example.com/p2", 200, "<a href='/p6'>6</a><a href='/p7'>7</a>");
    fetcher->registerResponse("https://example.com/p3", 200, "<a href='/p8'>8</a><a href='/p9'>9</a>");
    for (int i = 4; i <= 9; ++i) {
        fetcher->registerResponse("https://example.com/p" + std::to_string(i), 200, "Leaf");
    }

    Config cfg1;
    cfg1.seedUrls = {"https://example.com/start"};
    cfg1.maxDepth = 3;
    cfg1.maxPages = 20;
    cfg1.concurrency = 1;

    CrawlerEngine engine1(cfg1, fetcher);
    engine1.run();

    auto pages1 = engine1.storage().getAllPages();
    std::set<std::string> set1;
    for (const auto& p : pages1) set1.insert(p.url);

    ASSERT_EQ(set1.size(), 10u);

    // Multi-threaded pool execution simulation
    WorkerPool pool(4);
    Config cfg4 = cfg1;
    cfg4.concurrency = 4;

    CrawlerEngine engine4(cfg4, fetcher);
    engine4.init();

    for (int i = 0; i < 4; ++i) {
        pool.enqueue([&engine4]() {
            int idleRounds = 0;
            while (!engine4.scheduler().isFinished() && idleRounds < 20) {
                if (engine4.step()) {
                    idleRounds = 0;
                } else {
                    idleRounds++;
                    std::this_thread::yield();
                }
            }
        });
    }
    pool.waitIdle();

    // Verify engine finished and crawled all pages
    ASSERT_EQ(engine4.crawledCount(), 10u);

}

TEST_CASE(TestHttpFetcherSsrfProtection) {
    HttpFetcher fetcher(false); // allowTestLoopback = false

    FetchRequest reqLoopback;
    reqLoopback.url = "http://127.0.0.1:80/index.html";

    auto rLoop = fetcher.fetch(reqLoopback);
    ASSERT_TRUE(rLoop.isError());
    ASSERT_EQ(rLoop.code(), StatusCode::SSRF_BLOCKED);

    FetchRequest reqMetadata;
    reqMetadata.url = "http://169.254.169.254/latest/meta-data/";

    auto rMeta = fetcher.fetch(reqMetadata);
    ASSERT_TRUE(rMeta.isError());
    ASSERT_EQ(rMeta.code(), StatusCode::SSRF_BLOCKED);
}
