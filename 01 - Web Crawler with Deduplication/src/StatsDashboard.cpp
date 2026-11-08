#include "StatsDashboard.h"
#include "ContentHasher.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace crawler {

CrawlStats StatsDashboard::collect(const CrawlerEngine& engine) {
    CrawlStats stats;
    stats.pagesCrawled = engine.crawledCount();
    stats.queueSize = engine.queueSize();
    stats.currentDepth = engine.scheduler().currentDepth();
    stats.elapsedSeconds = engine.elapsedSeconds();
    if (stats.elapsedSeconds > 0.0001) {
        stats.pagesPerSecond = static_cast<double>(stats.pagesCrawled) / stats.elapsedSeconds;
    }
    stats.totalBytesDownloaded = engine.totalBytesFetched();

    auto allPages = engine.storage().getAllPages();
    for (const auto& page : allPages) {
        stats.statusCounts[page.statusCode]++;
        stats.depthHistogram[page.depth]++;
    }

    stats.candidateUrlsTested = engine.candidateUrlsSeen();
    stats.bloomNegatives = engine.bloomNegatives();
    stats.bloomPositives = engine.bloomPositives();
    stats.exactDuplicates = engine.exactDuplicates();
    stats.bloomFalsePositives = engine.falsePositiveDetections();

    size_t totalQueries = stats.bloomNegatives + stats.bloomPositives;
    if (totalQueries > 0) {
        stats.empiricalFpr = static_cast<double>(stats.bloomFalsePositives) / static_cast<double>(totalQueries);
    } else {
        stats.empiricalFpr = 0.0;
    }

    stats.theoreticalFpr = engine.bloomFilter().theoreticalFpr();
    stats.bloomBitsSet = engine.bloomFilter().countBitsSet();
    stats.bloomTotalBits = engine.bloomFilter().bitCount();
    stats.bloomFillRatio = engine.bloomFilter().fillRatio();

    // Check near-duplicates across stored pages
    size_t nearDupes = 0;
    for (size_t i = 0; i < allPages.size(); ++i) {
        if (allPages[i].simHash == 0) continue;
        for (size_t j = i + 1; j < allPages.size(); ++j) {
            if (allPages[j].simHash == 0) continue;
            if (ContentHasher::isNearDuplicate(allPages[i].simHash, allPages[j].simHash, 3)) {
                nearDupes++;
                break; // counted for page i
            }
        }
    }
    stats.nearDuplicateCount = nearDupes;

    // Memory footprint estimation
    size_t bloomMem = stats.bloomTotalBits / 8;
    size_t visitedMem = engine.visitedSet().size() * 128;
    size_t queueMem = stats.queueSize * 96;
    size_t pageStorageMem = 0;
    for (const auto& page : allPages) {
        pageStorageMem += sizeof(CrawledPage) + page.url.size() + page.title.size() + page.cleanText.size();
        pageStorageMem += page.outLinks.size() * 64;
    }
    stats.estimatedMemoryBytes = sizeof(CrawlerEngine) + bloomMem + visitedMem + queueMem + pageStorageMem;

    return stats;
}

std::string StatsDashboard::formatBytes(size_t bytes) {
    std::ostringstream oss;
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) << (static_cast<double>(bytes) / 1024.0) << " KB";
    } else {
        oss << std::fixed << std::setprecision(2) << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MB";
    }
    return oss.str();
}

std::string StatsDashboard::progressBar(double fraction, int width) {
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    int filled = static_cast<int>(fraction * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) bar += "=";
        else if (i == filled) bar += ">";
        else bar += " ";
    }
    bar += "]";
    return bar;
}

std::string StatsDashboard::renderAscii(const CrawlStats& stats, bool useAnsiColors) {
    // Check NO_COLOR environment variable
    if (std::getenv("NO_COLOR") != nullptr) {
        useAnsiColors = false;
    }

    const std::string RESET  = useAnsiColors ? "\033[0m" : "";
    const std::string BOLD   = useAnsiColors ? "\033[1m" : "";
    const std::string CYAN   = useAnsiColors ? "\033[36m" : "";
    const std::string GREEN  = useAnsiColors ? "\033[32m" : "";
    const std::string YELLOW = useAnsiColors ? "\033[33m" : "";
    const std::string BLUE   = useAnsiColors ? "\033[34m" : "";
    const std::string MAGENTA= useAnsiColors ? "\033[35m" : "";

    std::ostringstream out;
    out << BOLD << CYAN << "================================================================================\n" << RESET;
    out << BOLD << "              WEB CRAWLER & DEDUPLICATION - REAL-TIME DASHBOARD\n" << RESET;
    out << BOLD << CYAN << "================================================================================\n" << RESET;

    int totalSeconds = static_cast<int>(stats.elapsedSeconds);
    int hrs = totalSeconds / 3600;
    int mins = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;
    int millis = static_cast<int>((stats.elapsedSeconds - totalSeconds) * 1000);

    out << "  " << BOLD << "Elapsed Time:" << RESET << " "
        << std::setfill('0') << std::setw(2) << hrs << ":"
        << std::setw(2) << mins << ":"
        << std::setw(2) << secs << "."
        << std::setw(3) << millis << std::setfill(' ')
        << "  |  " << BOLD << "Throughput:" << RESET << " "
        << GREEN << std::fixed << std::setprecision(1) << stats.pagesPerSecond << " pages/sec" << RESET
        << "  |  " << BOLD << "Data:" << RESET << " " << formatBytes(stats.totalBytesDownloaded) << "\n";
    out << "--------------------------------------------------------------------------------\n";

    // Progress metrics
    out << "  " << BOLD << "Pages Crawled:" << RESET << " " << GREEN << stats.pagesCrawled << RESET
        << "        " << BOLD << "Queue Depth:" << RESET << " " << YELLOW << stats.queueSize << RESET
        << "        " << BOLD << "Current BFS Level:" << RESET << " " << stats.currentDepth << "\n\n";

    // Depth distribution
    out << "  " << BOLD << "Depth Distribution:" << RESET << "\n";
    size_t maxDepthCount = 1;
    for (const auto& [d, cnt] : stats.depthHistogram) {
        if (cnt > maxDepthCount) maxDepthCount = cnt;
    }
    for (const auto& [d, cnt] : stats.depthHistogram) {
        double frac = static_cast<double>(cnt) / static_cast<double>(maxDepthCount);
        out << "    Level " << std::setw(2) << d << " " << progressBar(frac, 20)
            << " " << std::setw(6) << cnt << " pages\n";
    }
    if (stats.depthHistogram.empty()) {
        out << "    (No pages crawled yet)\n";
    }
    out << "\n";

    // HTTP Status Codes
    out << "  " << BOLD << "HTTP Status Codes:" << RESET << "\n    ";
    if (stats.statusCounts.empty()) {
        out << "(None)";
    } else {
        bool first = true;
        for (const auto& [code, cnt] : stats.statusCounts) {
            if (!first) out << " | ";
            first = false;
            std::string codeColor = GREEN;
            if (code >= 300 && code < 400) codeColor = BLUE;
            else if (code >= 400 && code < 500) codeColor = YELLOW;
            else if (code >= 500 || code < 0) codeColor = MAGENTA;

            out << codeColor << (code < 0 ? "ERR" : std::to_string(code)) << RESET
                << ": " << cnt;
        }
    }
    out << "\n\n";

    // Deduplication Pipeline
    out << "  " << BOLD << "Two-Stage Deduplication Engine:" << RESET << "\n";
    out << "    Candidates Tested:        " << stats.candidateUrlsTested << "\n";
    out << "    Bloom Definite New:       " << GREEN << stats.bloomNegatives << RESET << " (direct scheduler enqueue)\n";
    out << "    Bloom Probable Seen:      " << YELLOW << stats.bloomPositives << RESET << " (consulted Exact Visited Set)\n";
    out << "    Exact Duplicates Dropped: " << stats.exactDuplicates << " (redundant crawls prevented)\n";
    out << "    Bloom False Positives:    " << MAGENTA << stats.bloomFalsePositives << RESET
        << " (recovered without losing valid URLs!)\n";
    out << "    Empirical FPR:            " << std::fixed << std::setprecision(4) << (stats.empiricalFpr * 100.0) << "%\n";
    out << "    Theoretical Target FPR:   " << std::fixed << std::setprecision(4) << (stats.theoreticalFpr * 100.0) << "%\n";
    out << "    Bloom Bits Set:           " << stats.bloomBitsSet << " / " << stats.bloomTotalBits
        << " (" << std::fixed << std::setprecision(2) << (stats.bloomFillRatio * 100.0) << "% fill ratio)\n";
    out << "    Near-Duplicate Pages:     " << stats.nearDuplicateCount << " (SimHash distance <= 3)\n\n";

    // Memory
    out << "  " << BOLD << "Estimated Memory Footprint:" << RESET << " "
        << formatBytes(stats.estimatedMemoryBytes) << "\n";
    out << BOLD << CYAN << "================================================================================\n" << RESET;

    return out.str();
}

void StatsDashboard::printLive(const CrawlStats& stats, bool clearScreen) {
    if (clearScreen) {
        // ANSI clear screen and home cursor
        std::cout << "\033[2J\033[H";
    }
    std::cout << renderAscii(stats) << std::flush;
}

} // namespace crawler

