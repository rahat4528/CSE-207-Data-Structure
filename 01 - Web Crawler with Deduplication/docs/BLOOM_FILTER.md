# Bloom Filter Mathematics, Optimization & Deduplication Architecture

This document presents the mathematical foundation, hash function selection, optimization theorems, and empirical validation of the custom double-hashing Bloom Filter implemented in the **Web Crawler with Deduplication**.

---

## 1. Theoretical Foundation

A Bloom Filter is a space-efficient probabilistic data structure used to test set membership. Query responses have no false negatives ($x \in S \implies \text{True}$), but may return false positives ($x \notin S \implies \text{True}$ with probability $p$).

### 1.1 Probability Model
Given:
- $m$: Total number of bits in the bit array.
- $n$: Expected number of elements inserted into the filter.
- $k$: Number of independent hash functions used.

When an element is inserted, each hash function sets a bit to 1. The probability that a specific bit is **not** set by a particular hash function is:
$$1 - \frac{1}{m}$$

After inserting $n$ independent elements using $k$ hash functions, the probability that a specific bit remains 0 is:
$$\left(1 - \frac{1}{m}\right)^{kn} \approx e^{-\frac{kn}{m}}$$

Therefore, the probability that a specific bit is 1 (the bit fill ratio $\rho$) is:
$$\rho \approx 1 - e^{-\frac{kn}{m}}$$

A false positive occurs when all $k$ bits tested for an uninserted query element happen to be 1. The false positive probability $p$ is:
$$p \approx \left(1 - e^{-\frac{kn}{m}}\right)^k$$

---

## 2. Parameter Optimization

### 2.1 Optimal Number of Hash Functions ($k$)
To minimize the false positive rate $p$ for fixed $m$ and $n$, differentiate with respect to $k$ and set the derivative to zero:
$$k = \frac{m}{n} \ln 2 \approx 0.69315 \cdot \frac{m}{n}$$

At this optimal $k$, the bit array is exactly 50% filled with 1s ($\rho = 0.5$).

### 2.2 Optimal Bit Array Size ($m$)
Substituting the optimal $k$ back into the false positive probability equation:
$$p = \left(\frac{1}{2}\right)^k = 2^{-\frac{m}{n} \ln 2} = e^{-\frac{m}{n} (\ln 2)^2}$$

Solving for $m$:
$$m = -\frac{n \ln p}{(\ln 2)^2} \approx -\frac{n \ln p}{0.48045} \approx -2.08136 \cdot n \ln p$$

#### Example Design Table:
| Capacity ($n$) | Target FPR ($p$) | Required Bits ($m$) | Bits per Element ($m/n$) | Hash Count ($k$) |
| :---: | :---: | :---: | :---: | :---: |
| 10,000 | 1% (0.01) | 95,851 | 9.59 bits | 7 |
| 100,000 | 1% (0.01) | 958,506 | 9.59 bits | 7 |
| 100,000 | 0.1% (0.001) | 1,437,759 | 14.38 bits | 10 |
| 1,000,000 | 1% (0.01) | 9,585,059 | 9.59 bits | 7 |

---

## 3. Kirsch-Mitzenmacher Double-Hashing Optimization

Evaluating $k$ independent cryptographic hash functions (such as SHA-256 or MD5) for every URL incurs significant CPU overhead.

### 3.1 Theorem (Kirsch & Mitzenmacher, 2006)
Only **two** independent 64-bit hash functions, $h_1(x)$ and $h_2(x)$, are required to simulate $k$ hash functions without any asymptotic degradation in the false positive probability:
$$g_i(x) = \big(h_1(x) + i \cdot h_2(x)\big) \pmod m, \quad \text{for } i \in \{0, 1, \dots, k-1\}$$

### 3.2 Implemented Hash Functions
In this crawler:
1. **$h_1(x)$ - FNV-1a (Fowler-Noll-Vo 64-bit):**
   - High dispersion, rapid bit mixing, zero allocation.
   - Prime multiplier: `0x100000001b3ULL`.
   - Offset basis: `0xcbf29ce484222325ULL`.
2. **$h_2(x)$ - MurmurHash3 64-bit finalizer:**
   - Evaluated with a constant seed (`0x9747b28cULL`).
   - Avalanche characteristics with uniform distribution across high and low words.

---

## 4. Two-Stage Deduplication Guarantee

A critical flaw in naive Bloom filter crawlers is that a false positive results in permanently skipping a valid, uncrawled webpage.

### The Two-Stage Solution:
```
           Target URL
               |
               v
     +--------------------+
     | Stage 1:           |
     | Bloom Filter       |
     +--------------------+
          /          \
         / (0)        \ (1)
        /              \
       v                v
   [Definite New]  +--------------------+
   * Enqueue BFS   | Stage 2:           |
   * Add to Bloom  | Exact Visited Set  |
   * Add to Visited+--------------------+
                         /          \
                        / (Found)    \ (Not Found)
                       /              \
                      v                v
              [True Duplicate]  [False Positive Rescued!]
              * Discard URL     * Enqueue BFS
                                * Add to Visited
```

### Safety Result:
- **Zero False Negatives:** If Bloom returns 0, the element is guaranteed never seen.
- **Zero Lost Webpages:** If Bloom returns 1, the Exact Visited Set verifies whether it is a genuine duplicate. If not found, it is rescued immediately.
