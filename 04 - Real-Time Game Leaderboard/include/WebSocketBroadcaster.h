#ifndef WEBSOCKET_BROADCASTER_H
#define WEBSOCKET_BROADCASTER_H

#include "Common.h"
#include "Leaderboard.h"
#include <string>
#include <vector>
#include <mutex>

namespace LeaderboardEngine {

enum class BroadcastType {
    TOP_K_UPDATE,
    PLAYER_RANK_UPDATE,
    SEASON_RESET
};

struct BroadcastEvent {
    BroadcastType type;
    std::string boardId;
    uint64_t boardVersion;
    std::string message;
    timestamp_t timestamp;
};

class IWebSocketBroadcaster {
public:
    virtual ~IWebSocketBroadcaster() = default;

    virtual void broadcastTopK(const std::string& boardId, uint64_t version, const std::vector<LeaderboardEntry>& topK) = 0;
    virtual void broadcastPlayerRank(const std::string& playerId, size_t oldRank, size_t newRank, score_t score) = 0;
    virtual void broadcastSeasonReset(const std::string& seasonName) = 0;
};

class MockWebSocketBroadcaster : public IWebSocketBroadcaster {
public:
    MockWebSocketBroadcaster() = default;

    void broadcastTopK(const std::string& boardId, uint64_t version, const std::vector<LeaderboardEntry>& topK) override;
    void broadcastPlayerRank(const std::string& playerId, size_t oldRank, size_t newRank, score_t score) override;
    void broadcastSeasonReset(const std::string& seasonName) override;

    std::vector<BroadcastEvent> getEvents() const;
    size_t eventCount() const;
    void clear();

private:
    mutable std::mutex lock_;
    std::vector<BroadcastEvent> events_;
};

} // namespace LeaderboardEngine

#endif // WEBSOCKET_BROADCASTER_H
