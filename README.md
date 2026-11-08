# CSE 207: Data Structures (Official Lab Experiments & Projects)

**Institution:** East West University  
**Department:** Computer Science and Engineering  
**Course:** CSE 207 (Data Structures)  
**Instructor:** Dr. Anup Kumar Paul, Associate Professor  

---

## 📌 Repository "About" (for GitHub)

> **This repository strictly follows the East West University CSE 207 Data Structures curriculum by Dr. Anup Kumar Paul. It includes the Student Record Management System Mini Project using BST, comprehensive advanced projects (Project 01 through Project 05), and all 10 core Lab Experiments in strict sequential order.**

---

## 📂 Repository Directory Layout

```text
CSE-207-Data-Structure/
│
├── 00 - Mini Project - Student Record System/ # Mini Project: Student Record Management System (BST)
│   ├── Student.h & Student.cpp            # Student Record Entity (Encapsulation)
│   ├── Node.h & Node.cpp                  # BST Node (Left/Right pointers)
│   ├── StudentBST.h & StudentBST.cpp      # BST Core Algorithms (Insert, Search, Delete, Traversals)
│   ├── main.cpp                           # Interactive Console UI & Random Unique Roll Generator
│   ├── build.bat                          # One-click native build script
│   ├── CMakeLists.txt                     # Standard CMake build definition
│   ├── README.md                          # Comprehensive documentation
│   └── cover.pdf                          # Official Project Report Cover Page
│
├── 01 - Web Crawler with Deduplication/   # Project 01: Web Crawler with Deduplication (Search Engine Foundation)
│   ├── include/ & src/                    # Bloom Filter, Two-Stage Deduplication, BFS Scheduler, Link Graph
│   ├── tests/                             # 44 Automated Unit, Concurrency, Safety, & Export Tests
│   ├── python/ & java/                    # Secondary Python 3 & Tertiary Java 17 companion demos
│   ├── docs/                              # ARCHITECTURE.md, BLOOM_FILTER.md, BENCHMARKS.md, SAFETY.md
│   └── build.bat                          # One-click automated build and master test runner
│
├── 02 - Time-Travel Key-Value Store/       # Project 02: Time-Travel Key-Value Store (Versioned Storage Engine)
│   ├── include/ & src/                    # Immutable Versioning, Temporal Binary Search, WAL, Transactions
│   ├── tests/                             # 37 Automated Unit, Point-in-Time, Crash Safety, & Concurrency Tests
│   ├── python/ & java/                    # Secondary Python 3 & Tertiary Java 17 companion demos
│   ├── docs/                              # ARCHITECTURE.md, FORMAT.md, and BENCHMARKS.md
│   └── build.bat                          # One-click automated dual build and master test runner
│
├── 03 - File Compression Tool/            # Project 03: File Compression Tool (Huffman Coding Engine)
│   ├── include/ & src/                    # Greedy Huffman Tree, Min-Heap, Priority Queue, BitBuffer
│   ├── tests/                             # 36 Automated Unit, Round-Trip, Streaming, and Mutation Tests
│   ├── python/ & java/                    # Secondary Python & Tertiary Java companion demos
│   ├── docs/                              # ARCHITECTURE.md, FORMAT.md, and BENCHMARKS.md
│   └── build.bat                          # One-click automated build, test, and run batch runner
│
├── 04 - Real-Time Game Leaderboard/       # Project 04: Real-Time Game Leaderboard (Ranking Engine)
│   ├── include/ & src/                    # Fenwick Tree, Indexed Max Heap, Coordinate Compressor
│   ├── tests/                             # 21 Automated Unit & Concurrency Stress Tests
│   ├── python/ & java/                    # Secondary Python & Tertiary Java companion demos
│   ├── docs/                              # ARCHITECTURE.md & BENCHMARKS.md documentation
│   └── build.bat                          # One-click automated build and test runner
│
├── 05 - Dependency Resolver/              # Project 05: Mini Package Manager & Dependency Resolver
│   ├── include/ & src/                    # Dual-Adjacency DAG, SemVer 2.0, Kahn's BFS & Iterative DFS
│   ├── tests/                             # 10 Test Suites covering 46 Automated Unit Tests
│   ├── python/ & java/                    # Secondary Python & Tertiary Java companion demos
│   ├── examples/ & docs/                  # Sample registries, Architecture & Complexity documentation
│   └── build.bat                          # One-click automated build and test runner
│
└── [Lab Experiments 01 to 10]             # Core Lab Experiments (Located in Root)
    ├── 01_Searching_Algorithms.cpp        # a. Linear Search, b. Binary Search
    ├── 02_Sorting_Algorithms.cpp          # a. Bubble Sort, b. Selection Sort
    ├── 03_Matrix_Operations.cpp           # a. Addition, b. Subtraction, c. Multiplication, d. Transpose
    ├── 04_Singly_Linked_List_Operations.cpp # Insert/Delete (Start, End, Any Position), Traversal
    ├── 05_Stack_Operations.cpp            # a. Push, b. Pop, c. Peek, d. Print
    ├── 06_Stack_Applications.cpp          # a. Infix to Postfix, b. Postfix Evaluation
    ├── 07_Queue_Operations.cpp            # Linear Queue & Circular Queue (Enqueue, Dequeue, Print)
    ├── 08_Recursive_Functions.cpp         # Factorial, Fibonacci, 100 to 1 reverse, "JAVA" 100 times, Palindrome
    ├── 09_Binary_Search_Tree_Operations.cpp # Insert, 3-Case Delete, Search, Pre/In/Post/Level-order Traversals
    └── 10_Graph_Operations.cpp            # Adjacency List, a. Breadth First (BFT), b. Depth First (DFT)
```

---

## 🌟 Mini Project: Student Record Management System (BST)

Located in `00 - Mini Project - Student Record System/`:
- **Data Structure:** Authentic **Binary Search Tree (BST)** using Roll Number as unique key.
- **Rules:** Smaller roll numbers go left, larger roll numbers go right, duplicate roll numbers strictly rejected.
- **Operations:** Insert, Search, 3-Case Deletion (Leaf, 1 Child, 2 Children with Inorder Successor), Inorder (Sorted), Preorder, and Postorder traversals.

### How to Compile & Run the Mini Project:
```bash
cd "00 - Mini Project - Student Record System"
build.bat
bin\StudentRecordSystem.exe
```

---

## 🕸️ Project 01: Web Crawler with Deduplication (Search Engine Foundation)

Located in `01 - Web Crawler with Deduplication/`:
- **Core Engine:** High-performance, production-grade web crawler and link graph extractor in modern ISO C++20.
- **Data Structures & Algorithms:** Custom Kirsch-Mitzenmacher double-hashing **Bloom Filter** (FNV-1a + Murmur64), **Two-Stage Deduplication Engine** (zero valid URLs lost on false positives), RFC 3986 URL Normalizer, RFC 9309 Robots Protocol parser, SSRF protection guard with IP pinning, Level-Synchronous BFS Frontier Scheduler, 64-bit Charikar **SimHash** near-duplicate content clustering, Directed Link Graph analyzer (in/out degrees, authorities, cycle detection), and atomic `.tmp` state checkpointing with CRC32 verification.
- **Visualizers & Exporters:** Real-time ANSI terminal dashboard, RFC 4180 CSV, JSON, Graphviz DOT, interactive standalone HTML5 canvas viewer, and Markdown audit report.
- **Automated Tests:** 44 automated test cases covering normalization, double-hashing FPR, scheduler levels, SSRF security, concurrency barriers, state persistence, and reports.
- **Companion Languages:** Includes Python 3 demonstration (`python/crawler_demo.py`) and Java 17 demonstration (`java/CrawlerDemo.java`).

### How to Compile & Run Project 01:
```bash
cd "01 - Web Crawler with Deduplication"

# Run all 44 automated unit, safety, concurrency, and export tests
./build.bat

# Run the master test suite directly
./bin/test_all.exe

# Run the built-in deterministic simulation with live dashboard
./bin/crawler.exe --demo

# Crawl a live website (with polite concurrency and rate limits)
./bin/crawler.exe --seed https://example.com/ --depth 2 --pages 50
```

---

## ⏳ Project 02: Time-Travel Key-Value Store (Versioned Storage Engine)

Located in `02 - Time-Travel Key-Value Store/`:
- **Core Engine:** Fully functional versioned key-value storage engine in modern ISO C++20.
- **Data Structures & Algorithms:** Append-only immutable version log, temporal binary search ($O(\log V)$ point-in-time and historical queries), Write-Ahead Log (WAL) with CRC32 integrity verification, Multi-Version Concurrency Control (MVCC) with Read-Committed isolation, periodic delta snapshots, and differential time-range changelog inspection.
- **Automated Tests:** 37 automated test cases covering snapshot round-trips, crash recovery, concurrent transaction aborts/commits, and temporal boundary lookups.
- **Companion Languages:** Includes Python 3 demonstration (`python/timetravel_demo.py`) and Java 17 demonstration (`java/TimeTravelDemo.java`).

### How to Compile & Run Project 02:
```bash
cd "02 - Time-Travel Key-Value Store"

# Run all 37 automated unit, temporal, crash, and concurrency tests
./build.bat test

# Launch interactive CLI
./bin/timetravel.exe repl
```

---

## 🗜️ Project 03: File Compression Tool (Huffman Coding Engine)

Located in `03 - File Compression Tool/`:
- **Core Engine:** Lossless data compression and decompression engine in modern ISO C++20.
- **Data Structures & Algorithms:** Custom **Binary Min-Heap** with strict total order tie-breaking, custom **Priority Queue**, greedy **Huffman Tree Builder** with iterative stack traversal, MSB-first **Bit Buffer** writer and reader, exact 0–7 bit padding handler, and fast 256-entry direct table + custom chained hash bucket frequency counter.
- **Features:** Self-contained `.huff` binary archive format, IEEE 802.3 CRC32 data integrity validation, atomic temporary file commits, directory traversal prevention (`../` and Windows device name sanitization), decompression bomb safety capping, dry-run entropy stats, batch directory processing, and terminal ASCII + standalone SVG vector tree visualization.
- **Automated Tests:** 36 automated test cases covering round-trip restoration (ASCII, Unicode BOM, binary, skewed, random), large-file 64 KB streaming, and 500 mutation fuzz tests.
- **Companion Languages:** Includes Python 3 demonstration (`python/huffman_demo.py`) and Java 17 demonstration (`java/HuffmanDemo.java`).

### How to Compile & Run Project 03:
```bash
cd "03 - File Compression Tool"

# Run all 36 automated unit, round-trip, streaming, and mutation tests
./build.bat test

# Compress a file
./bin/huffman.exe compress input.txt -o archive.huff

# Decompress an archive
./bin/huffman.exe decompress archive.huff -o restored_folder/

# Verify byte-for-byte exact identity
./bin/huffman.exe verify input.txt restored_folder/input.txt

# Launch interactive console REPL
./bin/huffman.exe repl
```

---

## 🏆 Project 04: Real-Time Game Leaderboard (Ranking Engine)

Located in `04 - Real-Time Game Leaderboard/`:
- **Core Engine:** Industrial-grade competitive ranking engine in modern C++17.
- **Data Structures & Algorithms:** Custom **Fenwick Tree (Binary Indexed Tree)** for prefix frequency and exact competition ranking ("1224" rule) in $O(\log S)$ time, custom **Indexed Max Heap** with position map for $O(1)$ peek and $O(\log N)$ updates, dynamic **Coordinate Compression** with sorted pending buffers, thread-safe read-write synchronization (`std::shared_mutex`), and Write-Ahead Log (WAL) with CRC32 checksums.
- **Features:** Multi-board time windows (Daily, Weekly, Monthly, All-Time), soft/hard seasonal resets, social friends leaderboard ($O(F \log F)$), head-to-head comparison, anti-cheat jump validator, and C++ ANSI live terminal dashboard.
- **Automated Tests:** 21 automated unit and concurrency stress tests.
- **Companion Languages:** Includes Python secondary demonstration (`python/leaderboard_demo.py`) and Java tertiary demonstration (`java/LeaderboardDemo.java`).

### How to Compile & Run Project 04:
```bash
cd "04 - Real-Time Game Leaderboard"

# Run one-click build and all 21 automated unit and load tests
./build.bat

# Launch interactive CLI
./bin/test_all.exe top 10 arcade
./bin/test_all.exe player p1 arcade
./bin/test_all.exe health
```

---

## 📦 Project 05: Package Manager & Dependency Resolver (DAG, Topological Sort & SemVer)

Located in `05 - Dependency Resolver/`:
- **Core Engine:** Complete dependency resolution and package management engine in modern ISO C++20.
- **Data Structures & Algorithms:** Dual-Adjacency Directed Acyclic Graph (DAG), Iterative 3-State DFS Cycle Detection ($100{,}000+$ safe with exact loop path extraction), Deterministic Kahn's Algorithm (BFS) Topological Sorting with Min-Priority Queue, Backtracking Constraint Propagation Solver, SemVer 2.0.0 interval arithmetic, LIFO Rollback Stack, Write-Ahead Crash Journal, and Doubly-Linked History List.
- **Automated Tests:** 10 Test Suites covering 46 Automated Unit Tests.
- **Companion Languages:** Includes Python secondary demonstration (`python/resolver_demo.py`) and Java tertiary demonstration (`java/DependencyResolverDemo.java`).

### How to Compile & Run Project 05:
```bash
cd "05 - Dependency Resolver"

# Run one-click build and all 46 unit tests across 10 test suites
./build.bat

# Launch the interactive console REPL
./bin/pkg-mgr.exe repl

# Or run commands directly
./bin/pkg-mgr.exe resolve nextjs
./bin/pkg-mgr.exe install express
./bin/pkg-mgr.exe tree nextjs
./bin/pkg-mgr.exe doctor
```

---

## 🔬 Lab Experiments 01–10 (Core Curriculum)

The core lab experiments are preserved in the root directory in strict sequential order from Experiment 01 to Experiment 10:

| Exp # | File Name | Topics & Operations |
|:---:|---|---|
| **01** | **[01_Searching_Algorithms.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/01_Searching_Algorithms.cpp)** | Linear Search $O(n)$ and Binary Search $O(\log n)$ |
| **02** | **[02_Sorting_Algorithms.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/02_Sorting_Algorithms.cpp)** | Bubble Sort and Selection Sort |
| **03** | **[03_Matrix_Operations.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/03_Matrix_Operations.cpp)** | Matrix Addition, Subtraction, Multiplication, Transpose |
| **04** | **[04_Singly_Linked_List_Operations.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/04_Singly_Linked_List_Operations.cpp)** | Singly Linked List: Insert (Start/End/Middle), Delete (Start/End/Middle), Print |
| **05** | **[05_Stack_Operations.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/05_Stack_Operations.cpp)** | Stack: Push (Overflow), Pop (Underflow), Peek, Print |
| **06** | **[06_Stack_Applications.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/06_Stack_Applications.cpp)** | Infix to Postfix Conversion and Postfix Expression Evaluation |
| **07** | **[07_Queue_Operations.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/07_Queue_Operations.cpp)** | Linear Queue and Circular Queue with modulo arithmetic |
| **08** | **[08_Recursive_Functions.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/08_Recursive_Functions.cpp)** | Factorial, Fibonacci, Print 100 to 1 reverse, "JAVA" 100 times, Palindrome |
| **09** | **[09_Binary_Search_Tree_Operations.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/09_Binary_Search_Tree_Operations.cpp)** | BST: Insert, Delete (Leaf, 1 Child, 2 Children with Inorder Successor), Search, Traversals |
| **10** | **[10_Graph_Operations.cpp](file:///c:/porject%2003/DEPENDENCY%20RESOLVER/10_Graph_Operations.cpp)** | Graph: Breadth First Traversal (BFT / BFS) and Depth First Traversal (DFT / DFS) |

### How to Compile & Run Any Specific Lab Experiment:
```bash
# Example: Experiment 01 (Searching)
g++ -std=c++17 01_Searching_Algorithms.cpp -o exp1.exe
./exp1.exe

# Example: Experiment 09 (Binary Search Tree)
g++ -std=c++17 09_Binary_Search_Tree_Operations.cpp -o exp9.exe
./exp9.exe

# Example: Experiment 10 (Graph Traversals)
g++ -std=c++17 10_Graph_Operations.cpp -o exp10.exe
./exp10.exe
```

---

## 📜 License
Developed for academic coursework and advanced algorithmic research at East West University.
