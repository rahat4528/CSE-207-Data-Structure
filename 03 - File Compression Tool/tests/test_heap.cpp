#include "TestRunner.h"
#include "MinHeap.h"

using namespace HuffmanEngine;

TEST_CASE(test_min_heap_basic_and_order) {
    MinHeap<int> heap;
    ASSERT_TRUE(heap.empty());
    ASSERT_EQ(heap.size(), 0);

    heap.push(50);
    heap.push(10);
    heap.push(30);
    heap.push(5);
    heap.push(100);
    heap.push(20);

    ASSERT_EQ(heap.size(), 6);
    ASSERT_TRUE(heap.validateHeapInvariant());
    ASSERT_EQ(heap.top(), 5);

    // Popping should return in strictly non-decreasing order
    std::vector<int> expected = {5, 10, 20, 30, 50, 100};
    for (int exp : expected) {
        ASSERT_EQ(heap.pop(), exp);
        ASSERT_TRUE(heap.validateHeapInvariant());
    }

    ASSERT_TRUE(heap.empty());
}

TEST_CASE(test_min_heap_duplicates_and_ties) {
    MinHeap<int> heap;
    heap.push(42);
    heap.push(42);
    heap.push(42);
    heap.push(10);
    heap.push(10);

    ASSERT_EQ(heap.size(), 5);
    ASSERT_EQ(heap.pop(), 10);
    ASSERT_EQ(heap.pop(), 10);
    ASSERT_EQ(heap.pop(), 42);
    ASSERT_EQ(heap.pop(), 42);
    ASSERT_EQ(heap.pop(), 42);
    ASSERT_TRUE(heap.empty());
}
