#include "TrapDetector.h"
#include <vector>

namespace crawler {

TrapDetector::TrapDetector(size_t maxHostPages) : m_maxHostPages(maxHostPages) {}

bool TrapDetector::hasTooManySegments(std::string_view path, size_t maxSegments) {
    size_t count = 0;
    for (char c : path) {
        if (c == '/') count++;
    }
    return count > maxSegments;
}

bool TrapDetector::hasRepeatingSegments(std::string_view path, size_t threshold) {
    std::vector<std::string_view> segments;
    size_t start = 0;
    while (start < path.size()) {
        size_t nextSlash = path.find('/', start);
        std::string_view seg = (nextSlash == std::string_view::npos) 
            ? path.substr(start) 
            : path.substr(start, nextSlash - start);

        if (!seg.empty()) {
            segments.push_back(seg);
        }
        if (nextSlash == std::string_view::npos) break;
        start = nextSlash + 1;
    }

    if (segments.size() < threshold) return false;

    // Check for identical adjacent repeats: /a/a/a
    size_t adjacentCount = 1;
    for (size_t i = 1; i < segments.size(); ++i) {
        if (segments[i] == segments[i - 1]) {
            adjacentCount++;
            if (adjacentCount >= threshold) return true;
        } else {
            adjacentCount = 1;
        }
    }

    // Check for repeating patterns of length 2: /a/b/a/b/a/b
    if (segments.size() >= threshold * 2) {
        for (size_t i = 0; i + 3 < segments.size(); ++i) {
            if (segments[i] == segments[i + 2] && segments[i + 1] == segments[i + 3]) {
                return true;
            }
        }
    }

    return false;
}

bool TrapDetector::isTrap(const Url& url) {
    if (!url.isValid()) return true;

    // Check length
    if (url.toString().size() > Url::MAX_URL_LENGTH) return true;

    // Check segment depth
    if (hasTooManySegments(url.path(), 15)) return true;

    // Check repeating path loops
    if (hasRepeatingSegments(url.path(), 3)) return true;

    // Check per-host page cap
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hostPageCounts.find(url.host());
    if (it != m_hostPageCounts.end() && it->second >= m_maxHostPages) {
        return true;
    }

    return false;
}

void TrapDetector::recordPageCrawled(const std::string& host) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hostPageCounts[host]++;
}

void TrapDetector::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hostPageCounts.clear();
}

} // namespace crawler

