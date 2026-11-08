# Project 03: File Compression Tool (Huffman Coding Engine)

A complete, production-grade lossless data compression and decompression application in **ISO C++20**. It implements genuine greedy Huffman Coding, variable-length bit packing into bytes, self-contained endian-safe binary archiving (`.huff`), and tamper-evident integrity verification.

---

## 1. Core Capabilities

- **Greedy Huffman Algorithm:** Models input byte distributions, builds an optimal prefix-free binary tree via a custom min-heap, and generates variable-length prefix-free codes.
- **Exact Bitstream Packing:** Packs codes into bytes MSB-first, handling partial-byte flushes and explicit padding bits ($0 \dots 7$).
- **Byte-for-Byte Exact Restoration:** Guarantees bit-level identical restoration for ASCII text, multi-script Unicode (Bengali, Chinese, European, Emojis), CRLF/LF line endings, UTF-8 BOMs, and raw binary executables.
- **Self-Contained Binary Format:** Archives include magic signature (`"HUFF"`), format version, original file size, modification timestamp, frequency dictionary, header CRC32, packed payload, and trailing original data CRC32.
- **Defense-in-Depth:**
  - Prevents path traversal vulnerabilities (`../`, drive letters, Windows reserved names).
  - Decompression bomb protection: bounds allocation to a configurable safety cap (10 GB).
  - Atomic temporary files: writes to `.tmp.<pid>` and atomically commits only upon verified checksum match.
  - Never overwrites input files; enforces overwrite protection (`--force` required).
- **Rich CLI & Interactive REPL:** Subcommands for compression, decompression, byte verification, header inspection, frequency preview, dry-run entropy stats, batch processing, benchmarking, and ASCII/SVG tree visualization.

---

## 2. Directory Structure

```text
03 - File Compression Tool/
├── CMakeLists.txt                 # Cross-platform CMake build configuration
├── build.bat                      # One-click Windows build, test, and run script
├── README.md                      # Comprehensive user manual and documentation
├── docs/
│   ├── ARCHITECTURE.md            # Deep architectural pipeline, invariants, and complexity
│   ├── FORMAT.md                  # Binary archive specification and byte layouts
│   └── BENCHMARKS.md              # Empirical compression ratios and throughput tables
├── include/
│   ├── Common.h                   # Result<T>, ErrorCode, 256-bit BitCode, CRC32 engine
│   ├── IStream.h                  # Abstract stream interfaces (Memory, Limited, File)
│   ├── FileIO.h                   # Safe buffered binary file reader & atomic writer
│   ├── FrequencyCounter.h         # Direct 256-symbol table & custom hash map
│   ├── HuffmanNode.h              # Tree node structure & strict tie-breaking
│   ├── MinHeap.h                  # Custom array-based binary min heap
│   ├── PriorityQueue.h            # Priority queue wrapper for tree construction
│   ├── HuffmanTree.h              # Tree builder, iterative code generator, Kraft validator
│   ├── BitBuffer.h                # MSB-first bit writer and bit reader
│   ├── ArchiveHeader.h            # Header serialization, deserialization, CRC validation
│   ├── Compressor.h               # Two-pass streaming compression pipeline
│   ├── Decompressor.h             # Safe bitstream decompression & integrity verification
│   ├── PathSanitizer.h            # Traversal prevention and overwrite validation
│   ├── BatchProcessor.h           # Multi-file directory batch compression
│   ├── TreeVisualizer.h           # Console ASCII tree & standalone SVG generator
│   ├── BenchmarkSuite.h           # Synthetic dataset throughput and ratio benchmarks
│   └── CLI.h                      # Subcommands router, argument parser, and REPL
├── src/
│   ├── Common.cpp / FileIO.cpp / FrequencyCounter.cpp / HuffmanTree.cpp
│   ├── BitBuffer.cpp / ArchiveHeader.cpp / Compressor.cpp / Decompressor.cpp
│   ├── PathSanitizer.cpp / BatchProcessor.cpp / BenchmarkSuite.cpp / TreeVisualizer.cpp
│   ├── CLI.cpp
│   └── main.cpp                   # Application entrypoint
├── tests/
│   ├── TestRunner.h               # Zero-dependency test harness
│   ├── test_frequency.cpp         # Frequency counting & Shannon entropy tests
│   ├── test_heap.cpp              # Min heap, tie-breaking, and priority queue tests
│   ├── test_huffman_tree.cpp      # Tree builder, prefix-free, Kraft's inequality tests
│   ├── test_bit_buffer.cpp        # Bit packing, unpacking, and 0-7 padding tests
│   ├── test_archive.cpp           # Header serialization, corruption, and CRC32 tests
│   ├── test_roundtrip.cpp         # Exact byte-for-byte round-trip on all profiles
│   ├── test_streaming.cpp         # Buffered streaming execution on large disk files
│   ├── test_corrupted.cpp         # Bit-flip mutation, truncated payloads, bomb tests
│   ├── test_path_safety.cpp       # Traversal prevention, reserved names, overwrite tests
│   ├── test_batch_cli.cpp         # Batch compression, visualizer, and CLI tests
│   └── test_all.cpp               # Master test runner
├── python/
│   └── huffman_demo.py            # Python 3 reference demonstration
└── java/
    └── HuffmanDemo.java           # Java 17 reference demonstration
```

---

## 3. Build & Run Instructions

### 3.1 Quick Start with `build.bat` (Recommended on Windows)

```powershell
# Navigate to the project directory
cd "03 - File Compression Tool"

# 1. Run all 36 automated unit, round-trip, and invariant test cases
.\build.bat test

# 2. Compile application and display help
.\build.bat run --help

# 3. Launch interactive REPL console
.\build.bat run repl
```

### 3.2 Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
ctest --output-on-failure
```

---

## 4. CLI Command Reference

### Compress a File
```bash
bin/huffman.exe compress input.txt -o archive.huff
# With forced overwrite if target exists:
bin/huffman.exe compress input.txt -o archive.huff -f
# With automatic stored fallback if compression overhead would expand file:
bin/huffman.exe compress input.txt -o archive.huff --stored
```

### Decompress an Archive
```bash
bin/huffman.exe decompress archive.huff -o restored_folder/
```

### Verify Integrity
```bash
# Compare original file and decompressed file byte-for-byte
bin/huffman.exe verify input.txt restored.txt

# Or verify an archive directly against original in-memory
bin/huffman.exe verify input.txt archive.huff
```

### Inspect Archive Metadata
```bash
bin/huffman.exe info archive.huff
```

### Preview Frequency Table & Codes
```bash
bin/huffman.exe preview input.txt --top 15
```

### Dry-Run Statistics
```bash
bin/huffman.exe stats input.txt
```

### Batch Directory Compression
```bash
bin/huffman.exe batch data_folder/ -o compressed_folder/ -r -f
```

### Performance Benchmarks
```bash
bin/huffman.exe benchmark
```

### Visualize Huffman Tree
```bash
# Terminal ASCII tree:
bin/huffman.exe tree input.txt

# Standalone vector SVG export:
bin/huffman.exe tree input.txt --format svg -o tree_diagram.svg
```

---

## 5. Automated Test Coverage (36 Test Cases)

| Test Suite | Test Cases | Scope Verified | Status |
| :--- | :---: | :--- | :---: |
| `test_frequency.cpp` | 4 | Empty streams, single symbols, two-symbol entropy, uniform 256 bytes | **PASSED** |
| `test_heap.cpp` | 2 | Min-heap ordering, duplicate weights, strict tie-breaking | **PASSED** |
| `test_huffman_tree.cpp`| 3 | Single-symbol convention, Kraft's inequality, deterministic construction | **PASSED** |
| `test_bit_buffer.cpp` | 3 | All 8 padding amounts (0–7 bits), multi-byte codes, arbitrary bit lengths | **PASSED** |
| `test_archive.cpp` | 4 | Header serialization, CRC32 verification, signature checks, truncation | **PASSED** |
| `test_roundtrip.cpp` | 8 | Empty, single char, two symbols, all 256 bytes, Unicode/BOM, random bytes | **PASSED** |
| `test_streaming.cpp` | 1 | Multi-megabyte disk file streaming with bounded 64 KB memory | **PASSED** |
| `test_corrupted.cpp` | 4 | Payload bit flips, temp file cleanup on abort, bomb rejection, 500 mutation fuzz runs | **PASSED** |
| `test_path_safety.cpp`| 3 | Path traversal (`../`), Windows reserved names (`CON`), overwrite policy | **PASSED** |
| `test_batch_cli.cpp` | 3 | ASCII/SVG visualizers, benchmark suite execution, CLI argument router | **PASSED** |
| `test_all.cpp` (Smoke)| 1 | Precomputed CRC32 test vector (`0xCBF43926`) and 256-bit `BitCode` operations | **PASSED** |
| **Total** | **36** | **Comprehensive Full System Verification (100% Pass Rate)** | **PASSED** |

---

## 6. Companion Multi-Language Implementations

- **Python 3:** `python/huffman_demo.py` (verified working with Python's standard `heapq`).
- **Java 17:** `java/HuffmanDemo.java` (verified compiling and running with standard `java.util.PriorityQueue`).

---

## 7. Five Future Improvements

1. **Canonical Huffman Code Stored Header:** Store only canonical code lengths rather than full frequency pairs to reduce header size from $\approx 450$ bytes to $\approx 64$ bytes for small files.
2. **Table-Driven Multi-Bit Decoder:** Accelerate tree traversal by decoding up to 8 bits simultaneously via a direct 256-entry lookup table for short codewords.
3. **Adaptive (Dynamic) Huffman Coding (FGK / Vitter Algorithm):** Implement single-pass adaptive Huffman coding that updates symbol trees dynamically without requiring a two-pass scan.
4. **Deflate / LZ77 Pre-pass Pipeline:** Precede Huffman coding with an LZ77 sliding window dictionary match stage to form a full standard DEFLATE-compatible compressor.
5. **Multi-threaded Block Compression:** Divide gigabyte-scale files into independent 1 MB blocks and encode them concurrently across CPU cores via `std::jthread`.
