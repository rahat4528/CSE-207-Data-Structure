#include "WebSocketBroadcaster.h"
#include <sstream>

namespace LeaderboardEngine {

void MockWebSocketBroadcaster::broadcastTopK(const std::string& boardId, uint64_t version, const std::vector<LeaderboardEntry>& topK) {
    std::lock_guard<std::mutex> lock(lock_);
    std::ostringstream oss;
    oss << "TopK size: " << topK.size();
    events_.push_back({BroadcastType::TOP_K_UPDATE, boardId, version, oss.str(), 0});
}

void MockWebSocketBroadcaster::broadcastPlayerRank(const std::string& playerId, size_t oldRank, size_t newRank, score_t score) {
    std::lock_guard<std::mutex> lock(lock_);
    std::ostringstream oss;
    oss << "Player " << playerId << " rank moved " << oldRank << " -> " << newRank << " (score: " << score << ")";
    events_.push_back({BroadcastType::PLAYER_RANK_UPDATE, "", 0, oss.str(), 0});
}

void MockWebSocketBroadcaster::broadcastSeasonReset(const std::string& seasonName) {
    std::lock_guard<std::mutex> lock(lock_);
    events_.push_back({BroadcastType::SEASON_RESET, "", 0, "Season reset: " + seasonName, 0});
}

std::vector<BroadcastEvent> MockWebSocketBroadcaster::getEvents() const {
    std::lock_guard<std::mutex> lock(lock_);
    return events_;
}

size_t MockWebSocketBroadcaster::eventCount() const {
    std::lock_guard<std::mutex> lock(lock_);
    return events_.size();
}

void MockWebSocketBroadcaster::clear() {
    std::lock_guard<std::mutex> lock(lock_);
    events_.clear();
}

} // namespace LeaderboardEngine
