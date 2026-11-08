#ifndef FENWICK_TREE_H
#define FENWICK_TREE_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace LeaderboardEngine {

class FenwickTree {
public:
    explicit FenwickTree(size_t size = 0);

    void resize(size_t size);
    void clear();

    // 1-based index operations
    void add(size_t index, int64_t delta);
    int64_t prefix(size_t index) const;
    int64_t range(size_t left, size_t right) const;
    int64_t total() const;
    int64_t countGreater(size_t index) const;

    // Binary lifting to find the smallest 1-based index such that prefix(idx) >= k
    // k is 1-based count (1 <= k <= total())
    // Returns 0 if no such index exists (e.g. k > total() or total == 0)
    size_t select(int64_t k) const;

    size_t size() const { return size_; }

private:
    size_t size_;
    std::vector<int64_t> tree_;
};

} // namespace LeaderboardEngine

#endif // FENWICK_TREE_H
