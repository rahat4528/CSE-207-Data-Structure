#include "ExactVisitedSet.h"

namespace crawler {

bool ExactVisitedSet::insert(const std::string& url) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    return m_set.insert(url).second;
}

bool ExactVisitedSet::contains(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_set.find(url) != m_set.end();
}

size_t ExactVisitedSet::size() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_set.size();
}

void ExactVisitedSet::clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_set.clear();
}

std::vector<std::string> ExactVisitedSet::getAll() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return std::vector<std::string>(m_set.begin(), m_set.end());
}

} // namespace crawler

