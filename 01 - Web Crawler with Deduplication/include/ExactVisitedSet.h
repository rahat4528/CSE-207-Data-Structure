#ifndef CRAWLER_EXACT_VISITED_SET_H
#define CRAWLER_EXACT_VISITED_SET_H

#include <string>
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <mutex>

namespace crawler {

class ExactVisitedSet {
public:
    ExactVisitedSet() = default;

    // Returns true if URL was newly inserted; false if it already existed
    bool insert(const std::string& url);

    bool contains(const std::string& url) const;

    size_t size() const;
    void clear();

    std::vector<std::string> getAll() const;

private:
    std::unordered_set<std::string> m_set;
    mutable std::shared_mutex m_mutex;
};

} // namespace crawler

#endif // CRAWLER_EXACT_VISITED_SET_H

