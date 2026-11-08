#include "TestRunner.h"
#include "Leaderboard.h"
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>

using namespace LeaderboardEngine;

TEST_CASE(test_throughput_and_latency_benchmarks) {
    Leaderboard board("bench:all_time", "bench", TimeWindow::ALL_TIME);

    const int NUM_PLAYERS = 1000;
    const int NUM_OPERATIONS = 5000;

    std::mt19937 rng(999);
    std::uniform_int_distribution<int> pDist(1, NUM_PLAYERS);
    std::uniform_int_distribution<int> sDist(10, 100000);

    std::vector<double> latenciesUs;
    latenciesUs.reserve(NUM_OPERATIONS);

    auto startTotal = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        std::string pid = "p" + std::to_string(pDist(rng));
        score_t score = sDist(rng);
        ScoreEvent ev{"ev_" + std::to_string(i), pid, "bench", score, ScoreUpdateMode::SET, 1000 + i};

        auto t0 = std::chrono::high_resolution_clock::now();
        board.updateScore(ev);
        auto t1 = std::chrono::high_resolution_clock::now();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        latenciesUs.push_back(us);
    }

    auto endTotal = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(endTotal - startTotal).count();
    double opsPerSec = (NUM_OPERATIONS / totalMs) * 1000.0;

    std::sort(latenciesUs.begin(), latenciesUs.end());
    double p50 = latenciesUs[latenciesUs.size() * 50 / 100];
    double p95 = latenciesUs[latenciesUs.size() * 95 / 100];
    double p99 = latenciesUs[latenciesUs.size() * 99 / 100];

    std::cout << "\n  --- BENCHMARK RESULTS (" << NUM_OPERATIONS << " operations) ---\n"
              << "  Throughput : " << static_cast<int>(opsPerSec) << " ops/sec\n"
              << "  Latency p50: " << p50 << " us\n"
              << "  Latency p95: " << p95 << " us\n"
              << "  Latency p99: " << p99 << " us\n"
              << "  --------------------------------------------------\n";

    ASSERT_GT(opsPerSec, 5000.0); // Minimum throughput threshold
    ASSERT_LT(p95, 5000.0);       // Sub-5ms p95 latency
    ASSERT_TRUE(board.validateInvariant());
}
