cmake_minimum_required(VERSION 3.20)
project(WebCrawler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

include_directories(include)

set(CORE_SOURCES
    src/CRC32.cpp
    src/Logger.cpp
    src/Config.cpp
    src/Url.cpp
    src/UrlNormalizer.cpp
    src/BloomFilter.cpp
    src/ExactVisitedSet.cpp
    src/CrawlGraph.cpp
    src/FakeFetcher.cpp
    src/HtmlParser.cpp
    src/PageStorage.cpp
    src/BfsScheduler.cpp
    src/CrawlerEngine.cpp
    src/RobotsParser.cpp
    src/DomainPolicy.cpp
    src/SsrfGuard.cpp
    src/ContentHasher.cpp
    src/RateLimiter.cpp
    src/TrapDetector.cpp
    src/HttpFetcher.cpp
    src/WorkerPool.cpp
    src/CheckpointManager.cpp
    src/Exporter.cpp
    src/StatsDashboard.cpp
    src/ReportGenerator.cpp
    src/GraphVisualizer.cpp
)

set(TEST_SOURCES
    tests/test_stage1_skeleton.cpp
    tests/test_url_normalizer.cpp
    tests/test_bloom_graph.cpp
    tests/test_parser_fetcher.cpp
    tests/test_scheduler_pipeline.cpp
    tests/test_policies_safety.cpp
    tests/test_concurrency_fetcher.cpp
    tests/test_persistence_export.cpp
    tests/test_dashboard_reports.cpp
)


add_executable(test_all
    ${TEST_SOURCES}
    ${CORE_SOURCES}
)

add_executable(crawler
    src/main.cpp
    ${CORE_SOURCES}
)

if(WIN32)
    target_link_libraries(test_all ws2_32)
    target_link_libraries(crawler ws2_32)
endif()


