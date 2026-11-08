#include "TestRunner.h"
#include "LeaderboardManager.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>

using namespace LeaderboardEngine;

TEST_CASE(test_concurrent_writes_and_reads) {
    auto clock = std::make_shared<SystemClock>();
    LeaderboardManager mgr(clock);

    const int NUM_PLAYERS = 20;
    for (int i = 1; i <= NUM_PLAYERS; ++i) {
        mgr.registerPlayer("p" + std::to_string(i), "User_" + std::to_string(i));
    }

    std::atomic<bool> startFlag{false};
    std::atomic<int> totalWrites{0};
    std::atomic<int> totalReads{0};

    // 4 Writer Threads
    std::vector<std::thread> writers;
    for (int w = 0; w < 4; ++w) {
        writers.emplace_back([&, w]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            std::mt19937 rng(42 + w);
            std::uniform_int_distribution<int> pDist(1, NUM_PLAYERS);
            std::uniform_int_distribution<int> sDist(10, 500);

            for (int i = 0; i < 50; ++i) {
                int pId = pDist(rng);
                std::string eid = "w_" + std::to_string(w) + "_ev_" + std::to_string(i);
                mgr.submitScore({eid, "p" + std::to_string(pId), "battle", sDist(rng), ScoreUpdateMode::SET, 0});
                totalWrites++;
            }
        });
    }

    // 2 Reader Threads
    std::vector<std::thread> readers;
    for (int r = 0; r < 2; ++r) {
        readers.emplace_back([&, r]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            auto board = mgr.getBoard("battle", TimeWindow::ALL_TIME);
            for (int i = 0; i < 40; ++i) {
                auto top5 = board->getTopK(5);
                totalReads++;
                std::this_thread::yield();
            }
        });
    }

    startFlag.store(true);

    for (auto& th : writers) th.join();
    for (auto& th : readers) th.join();

    ASSERT_EQ(totalWrites.load(), 200);
    ASSERT_EQ(totalReads.load(), 80);

    auto board = mgr.getBoard("battle", TimeWindow::ALL_TIME);
    ASSERT_TRUE(board->validateInvariant());
}
