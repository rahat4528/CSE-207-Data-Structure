# Web Crawler with Deduplication (Search Engine Foundation)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-44%20Passed%20%7C%200%20Failed-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()

A high-performance, production-grade web crawler and link graph extractor engineered in pure **ISO C++20** with zero external dependencies. Designed as the crawling and deduplication foundation for modern web search engines.

---

## Key Highlights

- **Kirsch-Mitzenmacher Double-Hashing Bloom Filter:** Computes $k$ bit indices using only two 64-bit independent hashes (FNV-1a and Murmur64), minimizing CPU operations without asymptotic loss of false-positive guarantees.
- **Two-Stage Deduplication Pipeline:** Combines a fast probabilistic Bloom filter (Stage 1) with an Exact Visited Set (Stage 2). Guarantees **zero valid URLs are skipped on false positives** while preventing redundant fetches.
- **RFC 3986 URL Normalizer:** Idempotent URI parsing, percent-encoding canonicalization, uppercase scheme/host normalization, default port removal, dot-segment (`.` / `..`) resolution, tracking parameter stripping, and relative link resolution.
- **RFC 9309 Robots Protocol:** Strict adherence to robots exclusion standards, wildcard pattern matching, crawl-delay parsing, and `rel="nofollow"` compliance.
- **SSRF Defense Guard:** Automatic pre-connection DNS resolution pinning and rejection of loopback (`127.0.0.0/8`), private subnets (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`), cloud metadata endpoints (`169.254.169.254`), and IPv4-mapped IPv6 ranges.
- **Level-Synchronous BFS Scheduler:** Level-order exploration barrier guaranteeing breadth-first search properties and depth limits.
- **64-bit Charikar SimHash:** Near-duplicate content clustering using tokenized Hamming distance ($\le 3$).
- **Crawl Link Graph Analyzer:** Adjacency matrix tracking in/out degrees, identifying top authority pages, dead-end pages, orphan pages, and cycle detection via 3-color DFS.
- **Atomic State Persistence:** Checkpointing using `.tmp` files and atomic renaming, validated by IEEE 802.3 CRC32 checksums.
- **Comprehensive Exporters:** Clean RFC 4180 CSV, JSON, Graphviz DOT, interactive standalone HTML5 canvas viewer, and GitHub-flavored Markdown audit reports.
- **Multi-Language Demonstrations:** Companion implementations provided in Python 3 (`python/crawler_demo.py`) and Java 17 (`java/CrawlerDemo.java`).

---

## Directory Layout

```
01 - Web Crawler with Deduplication/
├── CMakeLists.txt                # CMake build configuration
├── build.bat                     # Windows MinGW-W64 build script
├── include/                      # C++20 Header declarations
│   ├── BfsScheduler.h            # Level-Synchronous BFS Frontier
│   ├── BloomFilter.h             # Kirsch-Mitzenmacher Double-Hashing Bloom Filter
│   ├── CRC32.h                   # Hardware-assisted / Table-driven CRC32
│   ├── CheckpointManager.h       # Atomic state checkpointing & CRC32 verification
│   ├── Common.h                  # Result<T>, StatusCode, shared types
│   ├── Config.h                  # Configuration & CLI parser
│   ├── ContentHasher.h           # FNV-1a & 64-bit SimHash near-duplicate detector
│   ├── CrawlGraph.h              # Directed link graph & cycle detector
│   ├── CrawlerEngine.h           # Master crawling orchestrator
│   ├── DomainPolicy.h            # SAME_HOST, SAME_DOMAIN & path filtering
│   ├── ExactVisitedSet.h         # Thread-safe in-memory visited arbiter
│   ├── Exporter.h                # CSV & JSON page/graph serializers
│   ├── FakeFetcher.h             # Deterministic mock HTTP fetcher
│   ├── GraphVisualizer.h         # DOT, Mermaid & Interactive HTML5 viewer
│   ├── HtmlParser.h              # Tolerant HTML state-machine tokenizer
│   ├── HttpFetcher.h             # Native socket HTTP client with SSRF guard
│   ├── IFetcher.h                # Fetcher abstract interface & responses
│   ├── Logger.h                  # Thread-safe color logging subsystem
│   ├── MiniTest.h                # Zero-dependency test framework
│   ├── PageStorage.h             # In-memory crawled page storage
│   ├── RateLimiter.h             # Per-host politeness rate limiter & backoff
│   ├── ReportGenerator.h         # Markdown audit, broken links & dupe reports
│   ├── RobotsParser.h            # RFC 9309 robots.txt parser
│   ├── SsrfGuard.h               # IP range validator & security filter
│   ├── StatsDashboard.h          # Real-time ASCII terminal dashboard
│   ├── TrapDetector.h            # Crawler trap & loop detector
│   ├── Url.h                     # RFC 3986 URL structure
│   ├── UrlNormalizer.h           # URL canonicalizer & relative resolver
│   └── WorkerPool.h              # Multi-threaded worker pool & barrier
├── src/                          # C++20 Implementations
│   ├── main.cpp                  # CLI executable entrypoint
│   └── ...                       # Corresponding .cpp implementations
├── tests/                        # 44 Comprehensive unit & integration tests
├── python/                       # Python 3 companion demonstration
│   └── crawler_demo.py
├── java/                         # Java 17 companion demonstration
│   └── CrawlerDemo.java
└── docs/                         # Detailed technical documentation
    ├── ARCHITECTURE.md           # Architecture overview & data flow
    ├── BLOOM_FILTER.md           # Mathematics, optimal sizing & theorems
    ├── BENCHMARKS.md             # Throughput, latency & scaling analysis
    └── SAFETY.md                 # Politeness, SSRF guard & trap defense
```

---

## Build & Test Instructions

### Prerequisites
- Modern C++ compiler supporting ISO C++20 (GCC 11+, Clang 13+, or MSVC 2019+).
- *Optional for companion demos:* Python 3.8+ and OpenJDK 17+.

### Building with Batch Script (Windows)
```cmd
cd "01 - Web Crawler with Deduplication"
build.bat
```
This builds both `bin/test_all.exe` and `bin/crawler.exe`.

### Building with CMake
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Running the Test Suite
```cmd
bin\test_all.exe
```
Output:
```
======================================================
 SUMMARY: 44 PASSED, 0 FAILED in 20 ms
======================================================
```

---

## CLI Usage

```text
Usage: bin/crawler.exe [OPTIONS]

Options:
  --seed <url>              Add seed URL to crawl (can be repeated)
  --depth <n>               Maximum crawl depth (default: 3)
  --pages <n>               Maximum pages to crawl (default: 100)
  --concurrency <n>         Worker thread count (default: 4)
  --delay <ms>              Politeness delay per host in ms (default: 1000)
  --domain-mode <mode>      ANY | SAME_HOST | SAME_DOMAIN (default: ANY)
  --bloom-capacity <n>      Expected unique items for Bloom filter (default: 100000)
  --bloom-fpr <p>           Target false positive probability (default: 0.01)
  --user-agent <str>        Custom HTTP User-Agent header
  --no-robots               Disable robots.txt parsing and enforcement
  --no-simhash              Disable 64-bit SimHash near-duplicate detection
  --checkpoint <path>       State file path for atomic checkpoint save/resume
  --export-json <path>      Export crawled pages to JSON (default: data/crawled_pages.json)
  --export-csv <path>       Export crawled pages to CSV (default: data/crawled_pages.csv)
  --export-dot <path>       Export link graph to Graphviz DOT (default: data/crawl_graph.dot)
  --export-html <path>      Export interactive HTML viewer (default: data/crawl_graph.html)
  --export-report <path>    Export Markdown audit report (default: data/crawl_report.md)
  --demo                    Execute built-in deterministic simulation
  --help, -h                Show this help message and exit
```

### Running the Built-in Simulation Demo
```cmd
bin\crawler.exe --demo
```
This simulates crawling an interconnected cluster of pages with cross-links, duplicates, 404 dead links, robots.txt rules, and generates all export formats.

### Running Multi-Language Companion Demos
```bash
# Python 3 Demo
python python/crawler_demo.py

# Java 17 Demo
java java/CrawlerDemo.java
```

---

## Output Artifacts

Running the crawler generates the following artifacts in `data/`:
1. `crawled_pages.json`: Full metadata, titles, clean text, hashes, status codes, and out-links.
2. `crawled_pages.csv`: RFC 4180 compliant spreadsheet export with formula injection neutralization.
3. `crawl_graph.dot`: Graphviz DOT graph with status-code color-coding.
4. `crawl_graph.html`: Standalone offline interactive HTML5 viewer with canvas force visualization, search, and sidebar.
5. `crawl_report.md`: Complete Markdown audit report containing KPI summaries, authority pages, broken link referrers, and SimHash near-duplicate clusters.
6. `crawler.state`: CRC32-verified atomic binary checkpoint for crash recovery.
