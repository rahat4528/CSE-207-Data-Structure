# Architecture Overview: Web Crawler with Deduplication

This document details the software architecture, component relationships, data flow, concurrency model, and fault-tolerance guarantees of the **Web Crawler with Deduplication (Search Engine Foundation)** written in ISO C++20.

---

## 1. System Architecture Diagram

```
+---------------------------------------------------------------------------------------+
|                                    CLI ENTRYPOINT                                     |
|                              (main.cpp / Config)                                      |
+---------------------------------------------------------------------------------------+
                                           |
                                           v
+---------------------------------------------------------------------------------------+
|                                    CRAWLER ENGINE                                     |
|  +---------------------------------------------------------------------------------+  |
|  |                           LEVEL-SYNCHRONOUS BFS SCHEDULER                       |  |
|  |     [Current Level Queue: d] --------------------> [Next Level Queue: d + 1]    |  |
|  +---------------------------------------------------------------------------------+  |
|          |                                                         ^                  |
|          | pop next URL                                            | enqueue valid    |
|          v                                                         | new URLs         |
|  +---------------------------+                     +-------------------------------+  |
|  |       IFETCHER            |                     |    TWO-STAGE DEDUPLICATION    |  |
|  | (HttpFetcher/FakeFetcher) |                     |    PIPELINE                   |  |
|  +---------------------------+                     +-------------------------------+  |
|          |                                                         ^                  |
|          | HTTP Response                                           | check target URLs|
|          v                                                         |                  |
|  +---------------------------+                     +-------------------------------+  |
|  |       HTML PARSER         | -- extracted links -| Stage 1: Bloom Filter (Fast)  |  |
|  |  (State-Machine Tokenizer)|                     | Stage 2: Exact Visited Set    |  |
|  +---------------------------+                     +-------------------------------+  |
|          |                                                         |                  |
|          | clean text & metadata                                   v                  |
|          v                                                 +---------------+          |
|  +---------------------------+                             |  CRAWL GRAPH  |          |
|  |       PAGE STORAGE        |                             | (Nodes/Edges) |          |
|  |  (SimHash / In-Memory DB) |                             +---------------+          |
|  +---------------------------+                                                        |
+---------------------------------------------------------------------------------------+
       |                                                                  |
       v                                                                  v
+-----------------------------+                           +-----------------------------+
|    CHECKPOINT MANAGER       |                           |      EXPORT & REPORTING     |
| (Atomic .tmp + CRC32 Verify)|                           | (JSON, CSV, DOT, HTML, MD)  |
+-----------------------------+                           +-----------------------------+
```

---

## 2. Core Subsystems

### 2.1 URL Frontier & Level-Synchronous BFS Scheduler
The frontier enforces a strict breadth-first traversal order level by level:
- **Level Separation:** Discovered links from depth $d$ are enqueued into `m_nextLevelQueue` with depth $d+1$.
- **Level Barrier:** When `m_currentLevelQueue` is exhausted, the scheduler increments `m_currentDepth` and swaps `m_nextLevelQueue` into the active slot.
- **Safety Limits:** Crawl depth is bounded by `maxDepth`, and total crawled pages are strictly capped by `maxPages`.

### 2.2 Two-Stage Deduplication Engine
Redundant crawls are prevented using a layered approach:
1. **Stage 1 (Bloom Filter):** Kirsch-Mitzenmacher double-hashing filter ($k$ hashes computed from FNV-1a and Murmur64).
   - If Bloom reports **Negative (0)**: The URL is definitely new. It is immediately inserted into both the Bloom Filter and the Exact Visited Set, and enqueued to the frontier.
   - If Bloom reports **Positive (1)**: The URL is a candidate duplicate. It proceeds to Stage 2.
2. **Stage 2 (Exact Visited Set):** In-memory hash set with mutex protection.
   - If the URL is in the visited set: Redundant crawl is avoided (duplicate discarded).
   - If the URL is **NOT** in the visited set: This represents a **Bloom False Positive**. The URL is rescued, added to both structures, and enqueued. **Zero valid URLs are lost.**

### 2.3 Resilient HTTP Fetcher & SSRF Guard
The HTTP fetching layer connects directly over native sockets:
- **SSRF Protection:** Resolves hostnames to IP addresses before connecting. Any loopback (`127.0.0.0/8`), private subnet (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`), cloud metadata (`169.254.169.254`), or IPv4-mapped IPv6 address is blocked prior to socket creation.
- **Redirect Tracking:** Follows HTTP 301/302 redirects up to `maxRedirects`, detecting infinite redirection cycles.
- **Per-Host Politeness:** Enforces polite crawl delays per target domain, honoring HTTP 429 / 503 `Retry-After` response headers.

### 2.4 Tolerant State-Machine HTML Parser
The custom HTML parser extracts structured content without external library bloat:
- Extracts `<title>`, `<meta name="description">`, `<meta name="robots">`, `<base href="...">`, and `<link rel="canonical">`.
- Respects `rel="nofollow"` attributes and `<meta name="robots" content="nofollow">`.
- Extracts clean body text by ignoring `<script>`, `<style>`, and markup tags, normalizing whitespace.

### 2.5 Link Graph Analysis
Maintains forward and reverse adjacency structures:
- Computes node in-degrees and out-degrees.
- Identifies **Authority Pages** (pages with highest incoming citations).
- Identifies **Orphan Pages** (in-degree = 0) and **Dead-End Pages** (out-degree = 0).
- Performs depth-first cycle detection using three-color graph coloring (White / Gray / Black).

### 2.6 Atomic Checkpointing & Export
- Checkpoints serialize the complete frontier, exact visited set, raw bit vector of the Bloom Filter, crawled page metadata, and graph edges.
- Atomicity is guaranteed by writing to a temporary file (`.tmp`) and executing an atomic rename.
- Data integrity is validated using a trailing IEEE 802.3 CRC32 checksum.
- Exporters produce RFC 4180 CSV, JSON, Graphviz DOT, interactive HTML5 canvas viewer, and Markdown audit reports.
