#!/usr/bin/env python3
"""
Project 04: Real-Time Game Leaderboard (Ranking Engine)
Secondary Python Implementation Demonstration
Demonstrates: Fenwick Tree (Binary Indexed Tree), Coordinate Compression, and Competition Ranking ("1224").
"""

class FenwickTree:
    def __init__(self, size: int):
        self.size = size
        self.tree = [0] * (size + 1)

    def add(self, index: int, delta: int):
        while index <= self.size:
            self.tree[index] += delta
            index += index & (-index)

    def prefix(self, index: int) -> int:
        s = 0
        while index > 0:
            s += self.tree[index]
            index -= index & (-index)
        return s

    def total(self) -> int:
        return self.prefix(self.size)

    def count_greater(self, index: int) -> int:
        return self.total() - self.prefix(index)


class CoordinateCompressor:
    def __init__(self, scores):
        self.distinct = sorted(list(set(scores)))
        self.score_to_idx = {s: i + 1 for i, s in enumerate(self.distinct)}

    def get_index(self, score: int) -> int:
        return self.score_to_idx.get(score, 0)

    def domain_size(self) -> int:
        return len(self.distinct)


def main():
    print("============================================================")
    print(" PROJECT 04: REAL-TIME GAME LEADERBOARD (Python Companion)")
    print("============================================================")

    # Worked example from specification: Scores 100, 90, 90, 80
    players = [
        ("Alice", 100),
        ("Bob", 90),
        ("Charlie", 90),
        ("David", 80)
    ]

    scores = [s for _, s in players]
    comp = CoordinateCompressor(scores)
    bit = FenwickTree(comp.domain_size())

    # Ingest scores into Fenwick Tree
    for name, s in players:
        idx = comp.get_index(s)
        bit.add(idx, 1)

    print("\n--- Competition Ranking Results ('1224' Rule) ---")
    for name, s in players:
        idx = comp.get_index(s)
        # Rank = 1 + count of players with strictly higher score
        rank = 1 + bit.count_greater(idx)
        print(f"  {name:10} Score: {s:<5} => Rank #{rank}")

    assert 1 + bit.count_greater(comp.get_index(100)) == 1
    assert 1 + bit.count_greater(comp.get_index(90)) == 2
    assert 1 + bit.count_greater(comp.get_index(80)) == 4
    print("\n[SUCCESS] Python companion verification completed cleanly.\n")

if __name__ == "__main__":
    main()
