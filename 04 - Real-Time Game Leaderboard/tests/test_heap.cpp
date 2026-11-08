#include "TestRunner.h"
#include "IndexedMaxHeap.h"
#include <random>
#include <algorithm>

using namespace LeaderboardEngine;

TEST_CASE(test_indexed_heap_basic_and_order) {
    IndexedMaxHeap heap;
    ASSERT_TRUE(heap.empty());

    // Higher score wins; ties broken by earlier timestamp; then lexical player ID
    heap.insert("playerB", 100, 1000);
    heap.insert("playerA", 100, 1000); // Equal score, equal time -> playerA wins
    heap.insert("playerC", 200, 2000); // Higher score wins outright
    heap.insert("playerD", 100, 500);  // Equal score, earlier time -> playerD wins over A and B

    ASSERT_EQ(heap.size(), 4);
    ASSERT_TRUE(heap.validateHeapInvariant());

    auto top1 = heap.pop();
    ASSERT_EQ(top1.playerId, "playerC");
    ASSERT_EQ(top1.score, 200);

    auto top2 = heap.pop();
    ASSERT_EQ(top2.playerId, "playerD"); // Earlier timestamp (500 < 1000)

    auto top3 = heap.pop();
    ASSERT_EQ(top3.playerId, "playerA"); // Lexical ID ("playerA" < "playerB")

    auto top4 = heap.pop();
    ASSERT_EQ(top4.playerId, "playerB");

    ASSERT_TRUE(heap.empty());
}

TEST_CASE(test_indexed_heap_dynamic_update_and_remove) {
    IndexedMaxHeap heap;
    heap.insert("p1", 50, 100);
    heap.insert("p2", 80, 100);
    heap.insert("p3", 30, 100);

    ASSERT_EQ(heap.top().playerId, "p2");

    // Dynamic increase
    heap.update("p1", 120, 100);
    ASSERT_EQ(heap.top().playerId, "p1");
    ASSERT_TRUE(heap.validateHeapInvariant());

    // Dynamic decrease
    heap.update("p1", 10, 100);
    ASSERT_EQ(heap.top().playerId, "p2");
    ASSERT_TRUE(heap.validateHeapInvariant());

    // Arbitrary removal
    heap.remove("p2");
    ASSERT_FALSE(heap.contains("p2"));
    ASSERT_EQ(heap.top().playerId, "p3");
    ASSERT_TRUE(heap.validateHeapInvariant());
}

TEST_CASE(test_indexed_heap_top_k) {
    IndexedMaxHeap heap;
    for (int i = 1; i <= 20; ++i) {
        heap.insert("p" + std::to_string(i), i * 10, 100);
    }

    auto top5 = heap.getTopK(5);
    ASSERT_EQ(top5.size(), 5);
    ASSERT_EQ(top5[0].score, 200);
    ASSERT_EQ(top5[1].score, 190);
    ASSERT_EQ(top5[2].score, 180);
    ASSERT_EQ(top5[3].score, 170);
    ASSERT_EQ(top5[4].score, 160);

    // Verify heap remains intact with 20 elements
    ASSERT_EQ(heap.size(), 20);
    ASSERT_TRUE(heap.validateHeapInvariant());
}
