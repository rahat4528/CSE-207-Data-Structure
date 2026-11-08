#include "ReportGenerator.h"
#include "ContentHasher.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <algorithm>

namespace crawler {

std::vector<BrokenLink> ReportGenerator::findBrokenLinks(const CrawlGraph& graph, const PageStorage& storage) {
    std::vector<BrokenLink> broken;
    auto allPages = storage.getAllPages();

    for (const auto& page : allPages) {
        if (page.statusCode >= 400 || page.statusCode < 0) {
            BrokenLink bl;
            bl.targetUrl = page.url;
            bl.statusCode = page.statusCode;
            bl.referringUrls = graph.getInEdges(page.url);
            broken.push_back(std::move(bl));
        }
    }
    return broken;
}

std::vector<DuplicateCluster> ReportGenerator::findNearDuplicateClusters(const PageStorage& storage, uint32_t maxDistance) {
    std::vector<DuplicateCluster> clusters;
    auto allPages = storage.getAllPages();
    std::vector<bool> assigned(allPages.size(), false);

    for (size_t i = 0; i < allPages.size(); ++i) {
        if (assigned[i] || allPages[i].simHash == 0) continue;

        DuplicateCluster cluster;
        cluster.representativeHash = allPages[i].simHash;
        cluster.urls.push_back(allPages[i].url);
        assigned[i] = true;

        bool hasNearMatches = false;
        for (size_t j = i + 1; j < allPages.size(); ++j) {
            if (assigned[j] || allPages[j].simHash == 0) continue;

            if (allPages[i].contentHash != 0 && allPages[i].contentHash == allPages[j].contentHash) {
                cluster.urls.push_back(allPages[j].url);
                assigned[j] = true;
                cluster.isExact = true;
                hasNearMatches = true;
            } else if (ContentHasher::isNearDuplicate(allPages[i].simHash, allPages[j].simHash, maxDistance)) {
                cluster.urls.push_back(allPages[j].url);
                assigned[j] = true;
                cluster.isExact = false;
                hasNearMatches = true;
            }
        }

        if (hasNearMatches) {
            clusters.push_back(std::move(cluster));
        }
    }

    return clusters;
}

std::vector<AuthorityPage> ReportGenerator::getTopAuthorityPages(const CrawlGraph& graph, const PageStorage& storage, size_t topN) {
    auto mostLinked = graph.getMostLinkedPages(topN);
    std::vector<AuthorityPage> authority;
    authority.reserve(mostLinked.size());

    for (const auto& [url, inDeg] : mostLinked) {
        AuthorityPage ap;
        ap.url = url;
        ap.inDegree = inDeg;
        ap.outDegree = graph.outDegree(url);

        auto pageOpt = storage.getPage(url);
        if (pageOpt.has_value()) {
            ap.depth = pageOpt->depth;
            ap.title = pageOpt->title;
        }
        authority.push_back(std::move(ap));
    }

    return authority;
}

std::string ReportGenerator::generateMarkdownReport(const CrawlStats& stats,
                                                    const CrawlGraph& graph,
                                                    const PageStorage& storage) {
    std::ostringstream md;

    md << "# Web Crawler Audit & Performance Report\n\n";
    md << "> Generated automatically by **Web Crawler with Deduplication (ISO C++20)**.\n\n";

    md << "## 1. Executive Summary\n\n";
    md << "| Metric | Value |\n";
    md << "| :--- | :--- |\n";
    md << "| **Pages Crawled** | `" << stats.pagesCrawled << "` |\n";
    md << "| **Queue Remaining** | `" << stats.queueSize << "` |\n";
    md << "| **Current Max BFS Depth** | `" << stats.currentDepth << "` |\n";
    md << "| **Elapsed Time** | `" << std::fixed << std::setprecision(2) << stats.elapsedSeconds << " s` |\n";
    md << "| **Throughput** | `" << std::fixed << std::setprecision(2) << stats.pagesPerSecond << " pages/sec` |\n";
    md << "| **Total Data Downloaded** | `" << (static_cast<double>(stats.totalBytesDownloaded) / 1024.0) << " KB` |\n";
    md << "| **Graph Nodes** | `" << graph.nodeCount() << "` |\n";
    md << "| **Graph Edges** | `" << graph.edgeCount() << "` |\n";
    md << "| **Cycle Detected in Link Graph** | `" << (graph.hasCycle() ? "Yes" : "No") << "` |\n\n";

    md << "## 2. Two-Stage Deduplication Performance\n\n";
    md << "The crawler combines a Kirsch-Mitzenmacher double-hashing **Bloom Filter** as Stage 1 filter and an in-memory **Exact Visited Set** as Stage 2 arbiter:\n\n";
    md << "| Deduplication Metric | Result |\n";
    md << "| :--- | :--- |\n";
    md << "| **Candidate URLs Extracted** | `" << stats.candidateUrlsTested << "` |\n";
    md << "| **Stage 1 Bloom Negatives (Definite New)** | `" << stats.bloomNegatives << "` |\n";
    md << "| **Stage 1 Bloom Positives (Candidate Seen)** | `" << stats.bloomPositives << "` |\n";
    md << "| **Exact Duplicates Successfully Prevented** | `" << stats.exactDuplicates << "` |\n";
    md << "| **Bloom False Positives Detected & Rescued** | `" << stats.bloomFalsePositives << "` |\n";
    md << "| **Empirical False Positive Rate (FPR)** | `" << std::fixed << std::setprecision(4) << (stats.empiricalFpr * 100.0) << "%` |\n";
    md << "| **Theoretical Design Target FPR** | `" << std::fixed << std::setprecision(4) << (stats.theoreticalFpr * 100.0) << "%` |\n";
    md << "| **Bloom Bit Array Allocation** | `" << stats.bloomBitsSet << " / " << stats.bloomTotalBits << " bits (" << std::fixed << std::setprecision(2) << (stats.bloomFillRatio * 100.0) << "% fill)` |\n\n";

    md << "## 3. Depth Level Breakdown\n\n";
    md << "| Depth Level | Pages Crawled |\n";
    md << "| :---: | :---: |\n";
    for (const auto& [depth, cnt] : stats.depthHistogram) {
        md << "| Level " << depth << " | " << cnt << " |\n";
    }
    if (stats.depthHistogram.empty()) {
        md << "| *None* | 0 |\n";
    }
    md << "\n";

    md << "## 4. HTTP Status Distribution\n\n";
    md << "| HTTP Status | Count | Description |\n";
    md << "| :---: | :---: | :--- |\n";
    for (const auto& [code, cnt] : stats.statusCounts) {
        std::string desc = "OK";
        if (code == 301) desc = "Moved Permanently";
        else if (code == 302) desc = "Found";
        else if (code == 403) desc = "Forbidden";
        else if (code == 404) desc = "Not Found";
        else if (code == 429) desc = "Too Many Requests (Rate Limited)";
        else if (code == 500) desc = "Internal Server Error";
        else if (code < 0) desc = "Connection / Network Failure";
        md << "| `" << (code < 0 ? "ERR" : std::to_string(code)) << "` | " << cnt << " | " << desc << " |\n";
    }
    md << "\n";

    md << "## 5. Top Authority Pages (Highest In-Degree)\n\n";
    auto authorities = getTopAuthorityPages(graph, storage, 10);
    if (authorities.empty()) {
        md << "*No link relations established.*\n\n";
    } else {
        md << "| In-Degree | Out-Degree | Depth | URL | Title |\n";
        md << "| :---: | :---: | :---: | :--- | :--- |\n";
        for (const auto& ap : authorities) {
            md << "| " << ap.inDegree << " | " << ap.outDegree << " | " << ap.depth
               << " | `" << ap.url << "` | " << (ap.title.empty() ? "(untitled)" : ap.title) << " |\n";
        }
        md << "\n";
    }

    md << "## 6. Broken Links Report\n\n";
    auto broken = findBrokenLinks(graph, storage);
    if (broken.empty()) {
        md << " No broken links or HTTP errors encountered.\n\n";
    } else {
        md << "| Target URL | Status Code | Referred By (Parent Pages) |\n";
        md << "| :--- | :---: | :--- |\n";
        for (const auto& bl : broken) {
            std::string referrers;
            if (bl.referringUrls.empty()) {
                referrers = "(seed or direct)";
            } else {
                for (size_t k = 0; k < bl.referringUrls.size(); ++k) {
                    if (k > 0) referrers += ", ";
                    referrers += "`" + bl.referringUrls[k] + "`";
                }
            }
            md << "| `" << bl.targetUrl << "` | `" << (bl.statusCode < 0 ? "ERR" : std::to_string(bl.statusCode))
               << "` | " << referrers << " |\n";
        }
        md << "\n";
    }

    md << "## 7. Near-Duplicate & Exact Duplicate Content Clusters\n\n";
    auto clusters = findNearDuplicateClusters(storage, 3);
    if (clusters.empty()) {
        md << " No duplicate or near-duplicate document clusters detected (all content sufficiently distinct).\n\n";
    } else {
        md << "| Cluster Type | Representative SimHash | Clustered Pages |\n";
        md << "| :--- | :---: | :--- |\n";
        for (const auto& cl : clusters) {
            std::string pagesList;
            for (size_t k = 0; k < cl.urls.size(); ++k) {
                if (k > 0) pagesList += "<br/>";
                pagesList += "`" + cl.urls[k] + "`";
            }
            md << "| " << (cl.isExact ? "**Exact Match**" : "Near Duplicate (Hamming <= 3)")
               << " | `0x" << std::hex << cl.representativeHash << std::dec << "` | "
               << pagesList << " |\n";
        }
        md << "\n";
    }

    return md.str();
}

bool ReportGenerator::exportMarkdownReport(const std::string& filepath,
                                          const CrawlStats& stats,
                                          const CrawlGraph& graph,
                                          const PageStorage& storage) {
    std::ofstream out(filepath, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << generateMarkdownReport(stats, graph, storage);
    return out.good();
}

} // namespace crawler

