#include "TestRunner.h"
#include "FenwickTree.h"
#include "CoordinateCompressor.h"
#include <random>

using namespace LeaderboardEngine;

TEST_CASE(test_fenwick_basic_ops) {
    FenwickTree bit(10);
    ASSERT_EQ(bit.total(), 0);

    bit.add(3, 5);
    bit.add(7, 2);
    bit.add(3, 1); // 3 has count 6, 7 has count 2

    ASSERT_EQ(bit.prefix(2), 0);
    ASSERT_EQ(bit.prefix(3), 6);
    ASSERT_EQ(bit.prefix(6), 6);
    ASSERT_EQ(bit.prefix(7), 8);
    ASSERT_EQ(bit.prefix(10), 8);
    ASSERT_EQ(bit.total(), 8);

    ASSERT_EQ(bit.range(4, 7), 2);
    ASSERT_EQ(bit.range(3, 7), 8);
    ASSERT_EQ(bit.countGreater(3), 2);
    ASSERT_EQ(bit.countGreater(7), 0);
}

TEST_CASE(test_fenwick_select_binary_lifting) {
    FenwickTree bit(5);
    // Indices: 1: 0, 2: 2, 3: 0, 4: 3, 5: 1 -> prefix sums: 0, 2, 2, 5, 6
    bit.add(2, 2);
    bit.add(4, 3);
    bit.add(5, 1);

    ASSERT_EQ(bit.select(1), 2);
    ASSERT_EQ(bit.select(2), 2);
    ASSERT_EQ(bit.select(3), 4);
    ASSERT_EQ(bit.select(4), 4);
    ASSERT_EQ(bit.select(5), 4);
    ASSERT_EQ(bit.select(6), 5);
    ASSERT_EQ(bit.select(7), 0); // Out of bounds
}

TEST_CASE(test_fenwick_oracle_randomized) {
    const size_t N = 100;
    FenwickTree bit(N);
    std::vector<int64_t> brute(N + 1, 0);

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> distIdx(1, N);
    std::uniform_int_distribution<int64_t> distDelta(1, 10);

    for (int step = 0; step < 500; ++step) {
        size_t idx = distIdx(rng);
        int64_t delta = distDelta(rng);
        bit.add(idx, delta);
        brute[idx] += delta;
    }

    int64_t running = 0;
    for (size_t i = 1; i <= N; ++i) {
        running += brute[i];
        ASSERT_EQ(bit.prefix(i), running);
    }
    ASSERT_EQ(bit.total(), running);
}

TEST_CASE(test_coordinate_compression) {
    CoordinateCompressor comp(4);
    std::vector<score_t> scores = {50, 100, 20, 100, 80};
    comp.build(scores);

    // Distinct: 20 (idx 1), 50 (idx 2), 80 (idx 3), 100 (idx 4)
    ASSERT_EQ(comp.domainSize(), 4);
    ASSERT_EQ(comp.getIndex(20), 1);
    ASSERT_EQ(comp.getIndex(50), 2);
    ASSERT_EQ(comp.getIndex(80), 3);
    ASSERT_EQ(comp.getIndex(100), 4);
    ASSERT_EQ(comp.getIndex(999), 0);

    ASSERT_EQ(comp.getScore(1), 20);
    ASSERT_EQ(comp.getScore(4), 100);

    // Online pending score handling
    comp.addPending(60);
    comp.addPending(120);
    ASSERT_EQ(comp.pendingCount(), 2);
    ASSERT_EQ(comp.countPendingGreater(50), 2); // 60, 120
    ASSERT_EQ(comp.countPendingGreater(80), 1); // 120
    ASSERT_EQ(comp.countPendingGreater(150), 0);
}
