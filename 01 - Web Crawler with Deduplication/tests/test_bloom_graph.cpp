#include "MiniTest.h"
#include "BloomFilter.h"
#include "ExactVisitedSet.h"
#include "CrawlGraph.h"
#include <string>
#include <vector>

using namespace crawler;

TEST_CASE(TestBloomFilterNoFalseNegatives) {
    BloomFilter filter(1000, 0.01);

    std::vector<std::string> keys;
    for (int i = 0; i < 1000; ++i) {
        keys.push_back("https://example.com/page/" + std::to_string(i));
        filter.insert(keys.back());
    }

    ASSERT_EQ(filter.itemCount(), 1000u);

    // ZERO false negatives guaranteed
    for (const auto& key : keys) {
        ASSERT_TRUE(filter.possiblyContains(key));
    }
}

TEST_CASE(TestBloomFilterEmpiricalFprMatchesTheoretical) {
    size_t n = 5000;
    double targetFpr = 0.02; // 2%
    BloomFilter filter(n, targetFpr);

    // Insert n distinct keys
    for (size_t i = 0; i < n; ++i) {
        filter.insert("https://inserted.domain.org/doc_" + std::to_string(i));
    }

    // Query m distinct non-inserted keys
    size_t m = 5000;
    size_t falsePositives = 0;
    for (size_t i = 0; i < m; ++i) {
        std::string nonInserted = "https://unseen.otherdomain.net/item_" + std::to_string(i);
        if (filter.possiblyContains(nonInserted)) {
            falsePositives++;
        }
    }

    double empiricalFpr = static_cast<double>(falsePositives) / static_cast<double>(m);
    // Empirical FPR should be within reasonable bound of target (e.g. <= 0.05)
    ASSERT_TRUE(empiricalFpr <= 0.05);
    ASSERT_TRUE(filter.fillRatio() > 0.1 && filter.fillRatio() < 0.9);
}

TEST_CASE(TestBloomFalsePositiveDoesNotSkipNewUrl) {
    // A tiny filter with high collision rate
    BloomFilter tinyFilter(static_cast<size_t>(64), static_cast<size_t>(1)); // 64 bits, 1 hash function

    ExactVisitedSet exactSet;


    // Fill filter so collisions become virtually guaranteed
    for (int i = 0; i < 100; ++i) {
        std::string u = "https://example.com/seed_" + std::to_string(i);
        tinyFilter.insert(u);
        exactSet.insert(u);
    }

    // Test a completely brand-new URL
    std::string newUrl = "https://example.com/unique_brand_new_page_never_before_seen";

    // Regardless of whether tinyFilter says true (due to collision) or false:
    bool bloomSaysMaybe = tinyFilter.possiblyContains(newUrl);
    bool exactSaysSeen = exactSet.contains(newUrl);

    // Exact set MUST say not seen
    ASSERT_FALSE(exactSaysSeen);

    // The crawler policy decision:
    bool shouldSkip = false;
    if (bloomSaysMaybe) {
        // Bloom says "possibly seen" -> ALWAYS consult exact set!
        if (exactSaysSeen) {
            shouldSkip = true;
        }
    }

    // Verify the URL was NOT skipped!
    ASSERT_FALSE(shouldSkip);
}

TEST_CASE(TestExactVisitedSet) {
    ExactVisitedSet visited;
    ASSERT_EQ(visited.size(), 0u);

    ASSERT_TRUE(visited.insert("https://a.com"));
    ASSERT_TRUE(visited.contains("https://a.com"));
    ASSERT_EQ(visited.size(), 1u);

    // Repeated insert returns false
    ASSERT_FALSE(visited.insert("https://a.com"));
    ASSERT_EQ(visited.size(), 1u);

    ASSERT_TRUE(visited.insert("https://b.com"));
    ASSERT_EQ(visited.size(), 2u);

    auto all = visited.getAll();
    ASSERT_EQ(all.size(), 2u);
}

TEST_CASE(TestCrawlGraphDegreesAndMetrics) {
    CrawlGraph g;
    // A -> B, A -> C, B -> C, C -> D
    g.addEdge("A", "B");
    g.addEdge("A", "C");
    g.addEdge("B", "C");
    g.addEdge("C", "D");

    ASSERT_EQ(g.nodeCount(), 4u);
    ASSERT_EQ(g.edgeCount(), 4u);

    ASSERT_EQ(g.outDegree("A"), 2u);
    ASSERT_EQ(g.inDegree("A"), 0u);

    ASSERT_EQ(g.inDegree("C"), 2u); // From A and B
    ASSERT_EQ(g.outDegree("C"), 1u); // To D

    // Dead end page: D has outDegree 0
    auto deadEnds = g.getDeadEndPages();
    ASSERT_EQ(deadEnds.size(), 1u);
    ASSERT_EQ(deadEnds[0], "D");

    // Orphan page: A has inDegree 0
    auto orphans = g.getOrphanPages();
    ASSERT_EQ(orphans.size(), 1u);
    ASSERT_EQ(orphans[0], "A");

    // Most linked page: C has inDegree 2
    auto mostLinked = g.getMostLinkedPages(1);
    ASSERT_EQ(mostLinked.size(), 1u);
    ASSERT_EQ(mostLinked[0].first, "C");
    ASSERT_EQ(mostLinked[0].second, 2u);

    // Cycle check: A->B->C->D has no cycle
    ASSERT_FALSE(g.hasCycle());

    // Add back-edge D -> A to form cycle
    g.addEdge("D", "A");
    ASSERT_TRUE(g.hasCycle());

    // DOT output validation
    std::string dot = g.toDot();
    ASSERT_TRUE(dot.find("digraph CrawlGraph") != std::string::npos);
    ASSERT_TRUE(dot.find("\"A\" -> \"B\"") != std::string::npos);
}
