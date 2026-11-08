#include "MiniTest.h"
#include "TimeTravelStore.h"
#include <thread>
#include <vector>
#include <atomic>
#include <string>

using namespace ttkv;

TEST_CASE(TestConcurrentReadsAndWritesStress) {
    TimeTravelStore store;

    // Pre-populate some keys
    for (int i = 0; i < 20; ++i) {
        store.set("key_" + std::to_string(i), "init_" + std::to_string(i));
    }

    std::atomic<bool> startFlag{false};
    std::atomic<bool> stopFlag{false};
    std::atomic<size_t> totalReads{0};
    std::atomic<size_t> totalWrites{0};
    std::atomic<size_t> successfulReads{0};

    const int kNumWriters = 4;
    const int kNumReaders = 8;
    const int kWritesPerThread = 150;

    std::vector<std::thread> writers;
    std::vector<std::thread> readers;

    // Launch writer threads
    for (int w = 0; w < kNumWriters; ++w) {
        writers.emplace_back([&, w]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            for (int i = 0; i < kWritesPerThread; ++i) {
                std::string key = "key_" + std::to_string((w * 10 + i) % 20);
                std::string val = "val_w" + std::to_string(w) + "_" + std::to_string(i);
                auto res = store.set(key, val, 0, "writer_" + std::to_string(w));
                if (res.isOk()) {
                    totalWrites++;
                }
            }
        });
    }

    // Launch reader threads
    for (int r = 0; r < kNumReaders; ++r) {
        readers.emplace_back([&, r]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            while (!stopFlag.load()) {
                std::string key = "key_" + std::to_string(r % 20);
                auto res = store.get(key);
                if (res.isOk() && res.value().status == QueryStatus::FOUND) {
                    successfulReads++;
                }
                totalReads++;

                // Also execute occasional point-in-time and history queries
                if (totalReads % 25 == 0) {
                    store.history(key, 5, 0);
                }
            }
        });
    }

    // Start benchmark
    startFlag.store(true);

    // Wait for writers to complete all writes
    for (auto& t : writers) {
        t.join();
    }

    // Stop readers
    stopFlag.store(true);
    for (auto& t : readers) {
        t.join();
    }

    ASSERT_EQ(totalWrites.load(), static_cast<size_t>(kNumWriters * kWritesPerThread));
    ASSERT_TRUE(totalReads.load() > 0);
    ASSERT_TRUE(successfulReads.load() > 0);

    // Verify all keys remain intact and valid
    for (int i = 0; i < 20; ++i) {
        auto res = store.get("key_" + std::to_string(i));
        ASSERT_TRUE(res.isOk());
        ASSERT_EQ(res.value().status, QueryStatus::FOUND);
    }
}
