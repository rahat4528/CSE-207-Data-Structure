#!/usr/bin/env python3
"""
Web Crawler with Deduplication (Search Engine Foundation) - Python 3 Demonstration
Companion implementation demonstrating:
  - Kirsch-Mitzenmacher double-hashing Bloom Filter
  - Two-stage deduplication pipeline (Bloom Filter + Exact Visited Set)
  - Level-synchronous BFS frontier
  - 64-bit Charikar SimHash near-duplicate content detection
  - Crawl Link Graph (degrees, cycles, authority ranking)
"""

import math
import hashlib
import re
from collections import deque
from typing import Dict, List, Set, Tuple, Optional


class BloomFilter:
    """Kirsch-Mitzenmacher double-hashing Bloom Filter."""

    def __init__(self, expected_items: int = 1000, target_fpr: float = 0.01):
        self.expected_items = max(1, expected_items)
        self.target_fpr = target_fpr
        # m = - (n * ln p) / (ln 2)^2
        self.m = int(math.ceil(- (self.expected_items * math.log(target_fpr)) / (math.log(2) ** 2)))
        # k = (m / n) * ln 2
        self.k = max(1, int(round((self.m / self.expected_items) * math.log(2))))
        self.bit_array = [False] * self.m
        self.count = 0

    def _hashes(self, key: str) -> Tuple[int, int]:
        # Kirsch-Mitzenmacher using SHA-256 for deterministic 64-bit halves
        digest = hashlib.sha256(key.encode('utf-8')).digest()
        h1 = int.from_bytes(digest[:8], byteorder='big')
        h2 = int.from_bytes(digest[8:16], byteorder='big')
        if h2 == 0:
            h2 = 1
        return h1, h2

    def insert(self, key: str):
        h1, h2 = self._hashes(key)
        for i in range(self.k):
            pos = (h1 + i * h2) % self.m
            self.bit_array[pos] = True
        self.count += 1

    def possibly_contains(self, key: str) -> bool:
        h1, h2 = self._hashes(key)
        for i in range(self.k):
            pos = (h1 + i * h2) % self.m
            if not self.bit_array[pos]:
                return False
        return True

    def fill_ratio(self) -> float:
        set_bits = sum(1 for b in self.bit_array if b)
        return set_bits / self.m if self.m > 0 else 0.0


class SimHash:
    """64-bit Charikar SimHash for near-duplicate detection."""

    @staticmethod
    def compute(text: str) -> int:
        words = re.findall(r'[a-zA-Z0-9]+', text.lower())
        if not words:
            return 0

        v = [0] * 64
        for word in words:
            # 64-bit hash of token
            h = int.from_bytes(hashlib.md5(word.encode('utf-8')).digest()[:8], byteorder='big')
            for bit in range(64):
                if (h >> bit) & 1:
                    v[bit] += 1
                else:
                    v[bit] -= 1

        fingerprint = 0
        for bit in range(64):
            if v[bit] > 0:
                fingerprint |= (1 << bit)
        return fingerprint

    @staticmethod
    def hamming_distance(h1: int, h2: int) -> int:
        x = h1 ^ h2
        return bin(x).count('1')

    @classmethod
    def is_near_duplicate(cls, h1: int, h2: int, max_dist: int = 3) -> bool:
        return cls.hamming_distance(h1, h2) <= max_dist


class CrawlGraph:
    """Directed link graph with cycle detection and degree computation."""

    def __init__(self):
        self.nodes: Set[str] = set()
        self.adj: Dict[str, List[str]] = {}
        self.rev_adj: Dict[str, List[str]] = {}

    def add_node(self, url: str):
        if url not in self.nodes:
            self.nodes.add(url)
            self.adj[url] = []
            self.rev_adj[url] = []

    def add_edge(self, from_url: str, to_url: str):
        self.add_node(from_url)
        self.add_node(to_url)
        if to_url not in self.adj[from_url]:
            self.adj[from_url].append(to_url)
            self.rev_adj[to_url].append(from_url)

    def in_degree(self, url: str) -> int:
        return len(self.rev_adj.get(url, []))

    def out_degree(self, url: str) -> int:
        return len(self.adj.get(url, []))

    def has_cycle(self) -> bool:
        # 0: unvisited, 1: visiting, 2: visited
        color = {n: 0 for n in self.nodes}

        def dfs(u: str) -> bool:
            color[u] = 1
            for v in self.adj.get(u, []):
                if color[v] == 1:
                    return True
                if color[v] == 0 and dfs(v):
                    return True
            color[u] = 2
            return False

        for n in self.nodes:
            if color[n] == 0:
                if dfs(n):
                    return True
        return False


class DemoCrawlerEngine:
    """Two-Stage Deduplication Crawler Demo Engine."""

    def __init__(self, seed_urls: List[str], max_depth: int = 2, max_pages: int = 20):
        self.seed_urls = seed_urls
        self.max_depth = max_depth
        self.max_pages = max_pages

        self.bloom = BloomFilter(expected_items=500, target_fpr=0.01)
        self.exact_visited: Set[str] = set()
        self.graph = CrawlGraph()
        self.crawled_pages: Dict[str, dict] = {}

        self.candidate_urls_tested = 0
        self.bloom_negatives = 0
        self.bloom_positives = 0
        self.exact_duplicates_prevented = 0
        self.bloom_false_positives = 0

        # Simulated in-memory web cluster
        self.simulated_web = {
            "http://python-demo.org/": {
                "title": "Search Engine Core",
                "text": "The Python search engine foundation implements level synchronous breadth first search traversal.",
                "links": ["http://python-demo.org/about", "http://python-demo.org/docs", "http://python-demo.org/news", "http://python-demo.org/404"]
            },
            "http://python-demo.org/about": {
                "title": "About Search Engine",
                "text": "Algorithmic data structures guarantee fast exact URL membership and low memory usage.",
                "links": ["http://python-demo.org/"]
            },
            "http://python-demo.org/docs": {
                "title": "Documentation",
                "text": "Modern search engines crawl the web level by level with Level-Synchronous BFS. A Bloom filter provides rapid approximate duplicate checks.",
                "links": ["http://python-demo.org/", "http://python-demo.org/docs/bloom", "http://python-demo.org/docs/near-copy"]
            },
            "http://python-demo.org/docs/bloom": {
                "title": "Bloom Filter Mathematics",
                "text": "Kirsch-Mitzenmacher optimization allows k hash values to be simulated using two hashes: gi(x) = h1(x) + i * h2(x) mod m.",
                "links": ["http://python-demo.org/docs"]
            },
            "http://python-demo.org/docs/near-copy": {
                "title": "Documentation Mirror",
                "text": "Modern search engines crawl the web level by level with Level-Synchronous BFS. A Bloom filter provides rapid approximate duplicate checks.",
                "links": ["http://python-demo.org/docs"]
            },
            "http://python-demo.org/news": {
                "title": "Latest News",
                "text": "Engineers deploy new high-throughput deduplication crawlers with SimHash clustering.",
                "links": ["http://python-demo.org/"]
            }
        }

    def run(self):
        print("================================================================================")
        print("   PYTHON 3 COMPANION DEMO: WEB CRAWLER WITH DEDUPLICATION")
        print("================================================================================")

        # Queue holds (url, depth)
        queue = deque()

        for seed in self.seed_urls:
            self.bloom.insert(seed)
            self.exact_visited.add(seed)
            self.graph.add_node(seed)
            queue.append((seed, 0))

        pages_crawled = 0

        while queue and pages_crawled < self.max_pages:
            url, depth = queue.popleft()
            pages_crawled += 1

            if url in self.simulated_web:
                page_data = self.simulated_web[url]
                status = 200
                title = page_data["title"]
                text = page_data["text"]
                links = page_data.get("links", [])
            else:
                status = 404
                title = "Not Found"
                text = "Error: 404 Not Found"
                links = []

            sim_hash = SimHash.compute(text)
            self.crawled_pages[url] = {
                "status": status,
                "depth": depth,
                "title": title,
                "sim_hash": sim_hash,
                "text": text
            }

            print(f"[{status}] (Depth {depth}) Crawled: {url} -> \"{title}\"")

            if depth < self.max_depth and status == 200:
                for target in links:
                    self.graph.add_edge(url, target)
                    self.candidate_urls_tested += 1

                    # Two-stage deduplication
                    if not self.bloom.possibly_contains(target):
                        # Definite new URL
                        self.bloom_negatives += 1
                        self.bloom.insert(target)
                        self.exact_visited.add(target)
                        queue.append((target, depth + 1))
                    else:
                        # Probable duplicate: verify with exact set
                        self.bloom_positives += 1
                        if target not in self.exact_visited:
                            # Bloom false positive recovered
                            self.bloom_false_positives += 1
                            self.exact_visited.add(target)
                            queue.append((target, depth + 1))
                        else:
                            self.exact_duplicates_prevented += 1

        print("\n================================================================================")
        print("                        CRAWL EXECUTION METRICS")
        print("================================================================================")
        print(f"Total Pages Crawled:          {pages_crawled}")
        print(f"Graph Nodes Discovered:       {len(self.graph.nodes)}")
        print(f"Graph Edge Count:             {sum(len(v) for v in self.graph.adj.values())}")
        print(f"Link Graph Cycles Detected:   {self.graph.has_cycle()}")
        print("\n--- Two-Stage Deduplication Summary ---")
        print(f"Candidate Links Extracted:    {self.candidate_urls_tested}")
        print(f"Bloom Negatives (Definite):   {self.bloom_negatives}")
        print(f"Bloom Positives (Candidate):  {self.bloom_positives}")
        print(f"Exact Duplicates Prevented:   {self.exact_duplicates_prevented}")
        print(f"Bloom False Positives:        {self.bloom_false_positives}")
        print(f"Bloom Bit Array Fill Ratio:   {self.bloom.fill_ratio() * 100:.2f}%")

        print("\n--- SimHash Near-Duplicate Detection ---")
        page_list = list(self.crawled_pages.items())
        near_dupes_found = 0
        for i in range(len(page_list)):
            for j in range(i + 1, len(page_list)):
                u1, d1 = page_list[i]
                u2, d2 = page_list[j]
                if d1["status"] == 200 and d2["status"] == 200:
                    dist = SimHash.hamming_distance(d1["sim_hash"], d2["sim_hash"])
                    if dist <= 3:
                        print(f"  [Cluster Match] Distance = {dist}:")
                        print(f"    - {u1} ({d1['title']})")
                        print(f"    - {u2} ({d2['title']})")
                        near_dupes_found += 1

        if near_dupes_found == 0:
            print("  (No near-duplicate pages found)")

        print("================================================================================\n")


if __name__ == "__main__":
    crawler = DemoCrawlerEngine(["http://python-demo.org/"], max_depth=2, max_pages=15)
    crawler.run()

