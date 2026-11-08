#ifndef HUFFMAN_MIN_HEAP_H
#define HUFFMAN_MIN_HEAP_H

#include "Common.h"
#include <vector>
#include <utility>
#include <stdexcept>

namespace HuffmanEngine {

template <typename T, typename Comparator = std::less<T>>
class MinHeap {
private:
    std::vector<T> data;
    Comparator comp;

    void siftUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (comp(data[index], data[parent])) {
                std::swap(data[index], data[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(size_t index) {
        size_t n = data.size();
        while (true) {
            size_t smallest = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;

            if (left < n && comp(data[left], data[smallest])) {
                smallest = left;
            }
            if (right < n && comp(data[right], data[smallest])) {
                smallest = right;
            }

            if (smallest != index) {
                std::swap(data[index], data[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }

public:
    MinHeap() = default;
    explicit MinHeap(Comparator c) : comp(c) {}

    void push(const T& value) {
        data.push_back(value);
        siftUp(data.size() - 1);
    }

    void push(T&& value) {
        data.push_back(std::move(value));
        siftUp(data.size() - 1);
    }

    const T& top() const {
        if (data.empty()) {
            throw std::runtime_error("MinHeap::top() called on empty heap");
        }
        return data.front();
    }

    T pop() {
        if (data.empty()) {
            throw std::runtime_error("MinHeap::pop() called on empty heap");
        }
        T root = std::move(data.front());
        data.front() = std::move(data.back());
        data.pop_back();
        if (!data.empty()) {
            siftDown(0);
        }
        return root;
    }

    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    void clear() { data.clear(); }

    bool validateHeapInvariant() const {
        for (size_t i = 0; i < data.size(); ++i) {
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;
            if (left < data.size() && comp(data[left], data[i])) return false;
            if (right < data.size() && comp(data[right], data[i])) return false;
        }
        return true;
    }
};

} // namespace HuffmanEngine

#endif // HUFFMAN_MIN_HEAP_H
