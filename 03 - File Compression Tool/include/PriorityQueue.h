#ifndef HUFFMAN_PRIORITY_QUEUE_H
#define HUFFMAN_PRIORITY_QUEUE_H

#include "MinHeap.h"
#include "HuffmanNode.h"

namespace HuffmanEngine {

class HuffmanPriorityQueue {
private:
    MinHeap<HuffmanNode*, HuffmanNodeComparator> heap;

public:
    HuffmanPriorityQueue() = default;

    void push(HuffmanNode* node) {
        heap.push(node);
    }

    HuffmanNode* top() const {
        return heap.top();
    }

    HuffmanNode* pop() {
        return heap.pop();
    }

    size_t size() const {
        return heap.size();
    }

    bool empty() const {
        return heap.empty();
    }

    void clear() {
        heap.clear();
    }
};

} // namespace HuffmanEngine

#endif // HUFFMAN_PRIORITY_QUEUE_H
