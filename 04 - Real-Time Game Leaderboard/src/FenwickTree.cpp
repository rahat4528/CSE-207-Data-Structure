#include "FenwickTree.h"

namespace LeaderboardEngine {

FenwickTree::FenwickTree(size_t size) : size_(size), tree_(size + 1, 0) {}

void FenwickTree::resize(size_t size) {
    size_ = size;
    tree_.assign(size + 1, 0);
}

void FenwickTree::clear() {
    std::fill(tree_.begin(), tree_.end(), 0);
}

void FenwickTree::add(size_t index, int64_t delta) {
    if (index == 0 || index > size_) {
        throw std::out_of_range("FenwickTree: index out of bounds for add");
    }
    for (; index <= size_; index += index & -index) {
        tree_[index] += delta;
    }
}

int64_t FenwickTree::prefix(size_t index) const {
    if (index > size_) {
        index = size_;
    }
    int64_t sum = 0;
    for (; index > 0; index -= index & -index) {
        sum += tree_[index];
    }
    return sum;
}

int64_t FenwickTree::range(size_t left, size_t right) const {
    if (left == 0 || left > right || right > size_) return 0;
    return prefix(right) - prefix(left - 1);
}

int64_t FenwickTree::total() const {
    return prefix(size_);
}

int64_t FenwickTree::countGreater(size_t index) const {
    return total() - prefix(index);
}

size_t FenwickTree::select(int64_t k) const {
    if (k <= 0 || k > total() || size_ == 0) {
        return 0;
    }

    size_t idx = 0;
    int64_t currentSum = 0;

    // Find highest power of 2 <= size_
    size_t mask = 1;
    while ((mask << 1) <= size_) {
        mask <<= 1;
    }

    for (; mask > 0; mask >>= 1) {
        size_t nextIdx = idx + mask;
        if (nextIdx <= size_ && (currentSum + tree_[nextIdx] < k)) {
            idx = nextIdx;
            currentSum += tree_[nextIdx];
        }
    }

    return idx + 1;
}

} // namespace LeaderboardEngine
