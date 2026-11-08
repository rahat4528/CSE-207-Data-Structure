@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   Building Web Crawler with Deduplication (ISO C++20)
echo ============================================================

set CXX=g++
set CXXFLAGS=-std=c++20 -O2 -Wall -Wextra -Wpedantic -Wconversion -Iinclude

if not exist bin mkdir bin
if not exist data mkdir data

set CORE_SRCS=src/CRC32.cpp src/Logger.cpp src/Config.cpp src/Url.cpp src/UrlNormalizer.cpp src/BloomFilter.cpp src/ExactVisitedSet.cpp src/CrawlGraph.cpp src/FakeFetcher.cpp src/HtmlParser.cpp src/PageStorage.cpp src/BfsScheduler.cpp src/CrawlerEngine.cpp src/RobotsParser.cpp src/DomainPolicy.cpp src/SsrfGuard.cpp src/ContentHasher.cpp src/RateLimiter.cpp src/TrapDetector.cpp src/HttpFetcher.cpp src/WorkerPool.cpp src/CheckpointManager.cpp src/Exporter.cpp src/StatsDashboard.cpp src/ReportGenerator.cpp src/GraphVisualizer.cpp
set TEST_SRCS=tests/test_stage1_skeleton.cpp tests/test_url_normalizer.cpp tests/test_bloom_graph.cpp tests/test_parser_fetcher.cpp tests/test_scheduler_pipeline.cpp tests/test_policies_safety.cpp tests/test_concurrency_fetcher.cpp tests/test_persistence_export.cpp tests/test_dashboard_reports.cpp


echo Compiling test runner...
%CXX% %CXXFLAGS% %TEST_SRCS% %CORE_SRCS% -lws2_32 -o bin/test_all.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Test runner build failed!
    exit /b 1
)

echo Compiling main crawler application...
%CXX% %CXXFLAGS% src/main.cpp %CORE_SRCS% -lws2_32 -o bin/crawler.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Main application build failed!
    exit /b 1
)

echo [SUCCESS] Build completed: bin/test_all.exe and bin/crawler.exe

