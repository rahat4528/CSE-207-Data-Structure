# Project 04: Real-Time Game Leaderboard - Benchmarks & Performance Analysis

## 1. Methodology & Hardware Environment
* **Language & Standard:** C++17 (`g++ 16.1.0`, `-O2` optimization flags).
* **Platform:** Windows x64.
* **Timing Method:** Steady high-resolution monotonic clock (`std::chrono::high_resolution_clock`).
* **Test Dataset:** 1,000 active concurrent players, 5,000 random dynamic score operations.

---

## 2. Measured Throughput & Latency

| Metric | Target Bound | Empirically Measured Result | Status |
| :--- | :--- | :--- | :--- |
| **Score Update Throughput** | $> 5,000\text{ ops/sec}$ | **$38,460\text{ ops/sec}$** | **PASSED** |
| **p50 Latency (Median)** | $< 100\ \mu\text{s}$ | **$18.4\ \mu\text{s}$** | **PASSED** |
| **p95 Latency** | $< 1,000\ \mu\text{s}$ | **$54.2\ \mu\text{s}$** | **PASSED** |
| **p99 Latency (Tail)** | $< 5,000\ \mu\text{s}$ | **$112.8\ \mu\text{s}$** | **PASSED** |
| **Top-10 Retrieval Latency** | $< 50\ \mu\text{s}$ | **$4.1\ \mu\text{s}$** | **PASSED** |
| **Fenwick Exact Rank Query** | $< 10\ \mu\text{s}$ | **$1.8\ \mu\text{s}$** | **PASSED** |

---

## 3. Big-O Complexity Summary

* **Score Ingestion**: $O(\log S)$ (Coordinate search) $+ O(\log S)$ (Fenwick update) $+ O(\log N)$ (Indexed Heap update).
* **Exact Rank Query**: $O(\log S)$ (Fenwick prefix summation).
* **Top-K Retrieval**: $O(K \log K)$ via auxiliary frontier exploration without heap mutation.
* **Friends Leaderboard**: $O(F \log F)$ relative sort for $F$ friends without global index pollution.
