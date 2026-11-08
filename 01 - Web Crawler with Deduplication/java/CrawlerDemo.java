package crawler;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;

/**
 * Web Crawler with Deduplication (Search Engine Foundation) - Java 17 Companion Demo
 * Demonstrates:
 *   - Kirsch-Mitzenmacher double-hashing Bloom Filter (java.util.BitSet)
 *   - Two-stage deduplication pipeline (Bloom Filter + Exact Visited Set)
 *   - Level-synchronous BFS frontier
 *   - 64-bit SimHash near-duplicate content clustering
 *   - Directed Link Graph (degrees, cycle detection)
 */
public class CrawlerDemo {

    public static class BloomFilter {
        private final int numBits;
        private final int numHashes;
        private final BitSet bits;
        private int count = 0;

        public BloomFilter(int expectedItems, double targetFpr) {
            expectedItems = Math.max(1, expectedItems);
            // m = - (n * ln p) / (ln 2)^2
            this.numBits = (int) Math.ceil(- (expectedItems * Math.log(targetFpr)) / Math.pow(Math.log(2), 2));
            // k = (m / n) * ln 2
            this.numHashes = Math.max(1, (int) Math.round(((double) this.numBits / expectedItems) * Math.log(2)));
            this.bits = new BitSet(this.numBits);
        }

        private long[] getHashes(String key) {
            try {
                MessageDigest md = MessageDigest.getInstance("SHA-256");
                byte[] bytes = md.digest(key.getBytes(StandardCharsets.UTF_8));
                long h1 = 0, h2 = 0;
                for (int i = 0; i < 8; i++) h1 = (h1 << 8) | (bytes[i] & 0xFFL);
                for (int i = 8; i < 16; i++) h2 = (h2 << 8) | (bytes[i] & 0xFFL);
                if (h2 == 0) h2 = 1;
                return new long[]{h1, h2};
            } catch (Exception e) {
                long h = key.hashCode();
                return new long[]{h, h * 31 + 17};
            }
        }

        public void insert(String key) {
            long[] h = getHashes(key);
            for (int i = 0; i < numHashes; i++) {
                long pos = (h[0] + (long) i * h[1]) % numBits;
                if (pos < 0) pos += numBits;
                bits.set((int) pos);
            }
            count++;
        }

        public boolean possiblyContains(String key) {
            long[] h = getHashes(key);
            for (int i = 0; i < numHashes; i++) {
                long pos = (h[0] + (long) i * h[1]) % numBits;
                if (pos < 0) pos += numBits;
                if (!bits.get((int) pos)) {
                    return false;
                }
            }
            return true;
        }

        public double fillRatio() {
            return (double) bits.cardinality() / numBits;
        }
    }

    public static class SimHash {
        public static long compute(String text) {
            String[] words = text.toLowerCase().split("\\W+");
            int[] v = new int[64];

            for (String word : words) {
                if (word.isEmpty()) continue;
                long h = 0xcbf29ce484222325L;
                for (byte b : word.getBytes(StandardCharsets.UTF_8)) {
                    h ^= b;
                    h *= 0x100000001b3L;
                }
                for (int i = 0; i < 64; i++) {
                    if (((h >> i) & 1) == 1) v[i]++;
                    else v[i]--;
                }
            }

            long simHash = 0;
            for (int i = 0; i < 64; i++) {
                if (v[i] > 0) simHash |= (1L << i);
            }
            return simHash;
        }

        public static int hammingDistance(long a, long b) {
            return Long.bitCount(a ^ b);
        }
    }

    public static class CrawlGraph {
        private final Set<String> nodes = new LinkedHashSet<>();
        private final Map<String, List<String>> adj = new HashMap<>();
        private final Map<String, List<String>> revAdj = new HashMap<>();

        public void addNode(String url) {
            if (nodes.add(url)) {
                adj.put(url, new ArrayList<>());
                revAdj.put(url, new ArrayList<>());
            }
        }

        public void addEdge(String from, String to) {
            addNode(from);
            addNode(to);
            if (!adj.get(from).contains(to)) {
                adj.get(from).add(to);
                revAdj.get(to).add(from);
            }
        }

        public int inDegree(String url) {
            return revAdj.getOrDefault(url, Collections.emptyList()).size();
        }

        public int outDegree(String url) {
            return adj.getOrDefault(url, Collections.emptyList()).size();
        }

        public boolean hasCycle() {
            Map<String, Integer> color = new HashMap<>();
            for (String n : nodes) color.put(n, 0); // 0 = unvisited

            for (String n : nodes) {
                if (color.get(n) == 0 && dfs(n, color)) {
                    return true;
                }
            }
            return false;
        }

        private boolean dfs(String u, Map<String, Integer> color) {
            color.put(u, 1); // 1 = visiting
            for (String v : adj.getOrDefault(u, Collections.emptyList())) {
                if (color.get(v) == 1) return true;
                if (color.get(v) == 0 && dfs(v, color)) return true;
            }
            color.put(u, 2); // 2 = visited
            return false;
        }
    }

    static record PageData(String title, String text, List<String> links) {}
    static record CrawlTask(String url, int depth) {}
    static record StoredPage(String url, int status, int depth, String title, long simHash) {}

    public static void main(String[] args) {
        System.out.println("================================================================================");
        System.out.println("     JAVA 17 COMPANION DEMO: WEB CRAWLER WITH DEDUPLICATION");
        System.out.println("================================================================================");

        Map<String, PageData> webCluster = new HashMap<>();
        webCluster.put("http://java-demo.org/", new PageData(
            "Java Search Engine Portal",
            "Modern search engines crawl the web level by level with Level-Synchronous BFS. A Bloom filter provides rapid approximate duplicate checks.",
            List.of("http://java-demo.org/docs", "http://java-demo.org/about", "http://java-demo.org/broken-link")
        ));
        webCluster.put("http://java-demo.org/about", new PageData(
            "About Enterprise Crawler",
            "Algorithmic data structures guarantee fast exact URL membership and low memory usage.",
            List.of("http://java-demo.org/")
        ));
        webCluster.put("http://java-demo.org/docs", new PageData(
            "Crawler Documentation",
            "Modern search engines crawl the web level by level with Level-Synchronous BFS. A Bloom filter provides rapid approximate duplicate checks.",
            List.of("http://java-demo.org/", "http://java-demo.org/docs/mirror")
        ));
        webCluster.put("http://java-demo.org/docs/mirror", new PageData(
            "Documentation Syndicate",
            "Modern search engines crawl the internet level by level with Level-Synchronous BFS. A Bloom filter provides rapid approximate duplicate checks.",
            List.of("http://java-demo.org/docs")
        ));

        BloomFilter bloom = new BloomFilter(1000, 0.01);
        Set<String> exactVisited = new HashSet<>();
        CrawlGraph graph = new CrawlGraph();
        Queue<CrawlTask> queue = new ArrayDeque<>();
        List<StoredPage> stored = new ArrayList<>();

        String seed = "http://java-demo.org/";
        bloom.insert(seed);
        exactVisited.add(seed);
        graph.addNode(seed);
        queue.add(new CrawlTask(seed, 0));

        int candidatesTested = 0;
        int bloomNegatives = 0;
        int bloomPositives = 0;
        int exactDuplicates = 0;
        int bloomFalsePositives = 0;

        while (!queue.isEmpty() && stored.size() < 20) {
            CrawlTask task = queue.poll();
            PageData data = webCluster.get(task.url);

            int status = (data != null) ? 200 : 404;
            String title = (data != null) ? data.title : "Not Found";
            String text = (data != null) ? data.text : "Error 404";
            long simHash = SimHash.compute(text);

            stored.add(new StoredPage(task.url, status, task.depth, title, simHash));
            System.out.printf("[%d] (Depth %d) Crawled: %s -> \"%s\"%n", status, task.depth, task.url, title);

            if (data != null && task.depth < 2) {
                for (String target : data.links) {
                    graph.addEdge(task.url, target);
                    candidatesTested++;

                    if (!bloom.possiblyContains(target)) {
                        bloomNegatives++;
                        bloom.insert(target);
                        exactVisited.add(target);
                        queue.add(new CrawlTask(target, task.depth + 1));
                    } else {
                        bloomPositives++;
                        if (!exactVisited.contains(target)) {
                            bloomFalsePositives++;
                            exactVisited.add(target);
                            queue.add(new CrawlTask(target, task.depth + 1));
                        } else {
                            exactDuplicates++;
                        }
                    }
                }
            }
        }

        System.out.println("\n================================================================================");
        System.out.println("                        CRAWL EXECUTION METRICS");
        System.out.println("================================================================================");
        System.out.printf("Total Pages Crawled:          %d%n", stored.size());
        System.out.printf("Graph Nodes Discovered:       %d%n", graph.nodes.size());
        System.out.printf("Link Graph Cycles Detected:   %b%n", graph.hasCycle());
        System.out.println("\n--- Two-Stage Deduplication Summary ---");
        System.out.printf("Candidate Links Extracted:    %d%n", candidatesTested);
        System.out.printf("Bloom Negatives (Definite):   %d%n", bloomNegatives);
        System.out.printf("Bloom Positives (Candidate):  %d%n", bloomPositives);
        System.out.printf("Exact Duplicates Prevented:   %d%n", exactDuplicates);
        System.out.printf("Bloom False Positives:        %d%n", bloomFalsePositives);
        System.out.printf("Bloom Bit Array Fill Ratio:   %.2f%%%n", bloom.fillRatio() * 100);

        System.out.println("\n--- SimHash Near-Duplicate Detection ---");
        boolean foundNear = false;
        for (int i = 0; i < stored.size(); i++) {
            for (int j = i + 1; j < stored.size(); j++) {
                StoredPage p1 = stored.get(i);
                StoredPage p2 = stored.get(j);
                if (p1.status == 200 && p2.status == 200) {
                    int dist = SimHash.hammingDistance(p1.simHash, p2.simHash);
                    if (dist <= 3) {
                        System.out.printf("  [Cluster Match] Distance = %d:%n    - %s (%s)%n    - %s (%s)%n",
                            dist, p1.url, p1.title, p2.url, p2.title);
                        foundNear = true;
                    }
                }
            }
        }
        if (!foundNear) {
            System.out.println("  (No near duplicates found)");
        }
        System.out.println("================================================================================\n");
    }
}

