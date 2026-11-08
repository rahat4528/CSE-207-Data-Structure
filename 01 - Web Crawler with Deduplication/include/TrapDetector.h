#ifndef CRAWLER_TRAP_DETECTOR_H
#define CRAWLER_TRAP_DETECTOR_H

#include "Common.h"
#include "Url.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>

namespace crawler {

class TrapDetector {
public:
    explicit TrapDetector(size_t maxHostPages = 500);

    bool isTrap(const Url& url);
    void recordPageCrawled(const std::string& host);

    static bool hasRepeatingSegments(std::string_view path, size_t threshold = 3);
    static bool hasTooManySegments(std::string_view path, size_t maxSegments = 15);

    void clear();

private:
    size_t m_maxHostPages{500};
    std::unordered_map<std::string, size_t> m_hostPageCounts;
    std::mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_TRAP_DETECTOR_H

