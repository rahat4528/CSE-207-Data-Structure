# Architectural Specification — Huffman Coding Engine

## 1. System Overview

The **Huffman Coding Engine** is a high-performance, modular, lossless data compression and decompression system written in standard **ISO C++20**. It implements genuine greedy Huffman Coding, variable-length bit packing, endian-safe binary archiving, and tamper-evident integrity verification.

```
+-----------------------------------------------------------------------------------------+
|                                    COMPRESSION PIPELINE                                 |
+-----------------------------------------------------------------------------------------+
| Input Stream -> FrequencyCounter -> MinHeap -> HuffmanTree -> BitWriter -> .huff Archive|
+-----------------------------------------------------------------------------------------+

+-----------------------------------------------------------------------------------------+
|                                   DECOMPRESSION PIPELINE                                |
+-----------------------------------------------------------------------------------------+
| .huff Archive -> Header Parser -> HuffmanTree Rebuild -> BitReader -> Output Stream     |
|                                                                     |                   |
|                                                          Data CRC32 Verification        |
+-----------------------------------------------------------------------------------------+
```

---

## 2. Core Modules and Responsibilities

| Module | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **Common** | `Common.h`, `Common.cpp` | `Result<T>` monad, `ErrorCode`, 256-bit `BitCode`, and IEEE 802.3 `CRC32` lookup engine. |
| **Stream I/O** | `IStream.h`, `FileIO.h`, `FileIO.cpp` | Abstract stream interfaces, memory buffers, 64 KB buffered file I/O, and atomic temp-file commit. |
| **Frequency Counter** | `FrequencyCounter.h`, `FrequencyCounter.cpp` | Direct 256-symbol frequency table, custom hashed bucket map, and Shannon entropy calculator. |
| **Min Heap & Priority Queue**| `MinHeap.h`, `PriorityQueue.h` | Custom dynamic binary min-heap with strict total order comparator. |
| **Huffman Tree** | `HuffmanNode.h`, `HuffmanTree.h`, `HuffmanTree.cpp` | Greedy tree builder, iterative stack-based code generator, and Kraft's inequality validator. |
| **Bit Buffer** | `BitBuffer.h`, `BitBuffer.cpp` | MSB-first variable-length bit packing into bytes, exact 0–7 bit padding flush, and bit unpacking. |
| **Archive Format** | `ArchiveHeader.h`, `ArchiveHeader.cpp` | Endian-safe binary serialization, magic signature validation, and header CRC32 check. |
| **Compressor** | `Compressor.h`, `Compressor.cpp` | Two-pass buffered compression orchestration, advance padding computation, and stored fallback. |
| **Decompressor** | `Decompressor.h`, `Decompressor.cpp` | Header validation, bomb protection cap, single/multi-symbol bitstream decoding, and CRC32 verification. |
| **Path Safety** | `PathSanitizer.h`, `PathSanitizer.cpp` | Directory traversal prevention (`../`), Windows reserved device names, and overwrite validation. |
| **Batch Processor** | `BatchProcessor.h`, `BatchProcessor.cpp` | Multi-file directory batch compression and decompression with error isolation. |
| **Visualizer & Benchmarks**| `TreeVisualizer.h`, `BenchmarkSuite.h` | Terminal ASCII tree box-drawing, standalone SVG vector diagram generation, and performance timer. |
| **CLI & REPL** | `CLI.h`, `CLI.cpp`, `main.cpp` | Subcommands parser and interactive console REPL. |

---

## 3. Algorithmic Complexity Analysis

| Pipeline Phase | Time Complexity | Auxiliary Space | Mathematical Basis |
| :--- | :---: | :---: | :--- |
| **Frequency Analysis** | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ | Single pass over $N$ bytes; fixed 256-entry frequency table. |
| **Min Heap Construction** | $\mathcal{O}(S \log S)$ | $\mathcal{O}(S)$ | $S \le 256$ distinct symbols pushed into custom min-heap. |
| **Tree Construction** | $\mathcal{O}(S \log S)$ | $\mathcal{O}(S)$ | Greedily merges two lowest-weight nodes $S - 1$ times. |
| **Code Generation** | $\mathcal{O}(S)$ | $\mathcal{O}(S)$ | Non-recursive stack-based depth-first tree traversal. |
| **Bitstream Encoding** | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ | Emits variable-length codes via 64 KB streaming buffer. |
| **Bitstream Decoding** | $\mathcal{O}(B)$ | $\mathcal{O}(1)$ | Traverses tree 1 bit at a time ($B = \sum \text{freq}_i \times \text{len}_i$ bits). |
| **Total Compression** | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ | Constant memory footprint ($< 1\text{ MB}$) regardless of file size. |
| **Total Decompression**| $\mathcal{O}(N)$ | $\mathcal{O}(1)$ | Constant memory footprint ($< 1\text{ MB}$) regardless of file size. |

*Where $N$ is input file size in bytes, $S \le 256$ is alphabet size, and $B \le 8N$ is payload bit count.*

---

## 4. Deterministic Tie-Breaking Rules

To ensure identical inputs produce mathematically identical archives across all compilers and architectures, ties in node merging are resolved by a strict total ordering:
1. **Weight Ascending:** $\text{weight}(A) < \text{weight}(B)$.
2. **Minimum Contained Symbol Ascending:** $\min(A) < \min(B)$.
3. **Creation ID Ascending:** $\text{id}(A) < \text{id}(B)$.

---

## 5. Mathematical Invariants & Verification

1. **Prefix-Free Property:**
   $$\forall i \neq j, \quad \text{code}(i) \not\sqsubseteq \text{code}(j)$$
2. **Kraft-McMillan Inequality:**
   $$\sum_{i=1}^{S} 2^{-l_i} \le 1$$
3. **Shannon Source Coding Theorem:**
   $$H(X) \le \bar{L} < H(X) + 1$$
   *Where $H(X) = -\sum p_i \log_2 p_i$ is Shannon entropy, and $\bar{L} = \sum p_i l_i$ is average codeword length.*
4. **Decompression Bomb Protection:**
   The decompressor rejects archives with declared sizes exceeding `MAX_DECOMPRESSION_SIZE` (10 GB) before memory allocation.
5. **Path Traversal Security:**
   Archive extraction strips directory paths, Windows drive letters, and device names (`CON`, `PRN`, `AUX`, `NUL`, `COM1-9`, `LPT1-9`).
