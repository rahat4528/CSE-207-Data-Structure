#ifndef LEADERBOARD_MANAGER_H
#define LEADERBOARD_MANAGER_H

#include "Common.h"
#include "Player.h"
#include "Leaderboard.h"
#include "ScoreEvent.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <shared_mutex>

namespace LeaderboardEngine {

struct SeasonStandings {
    std::string seasonName;
    timestamp_t archivedAt;
    std::vector<LeaderboardEntry> topEntries;
};

class LeaderboardManager {
public:
    explicit LeaderboardManager(std::shared_ptr<IClock> clock = std::make_shared<SystemClock>());

    // Player Management
    Result<void> registerPlayer(const std::string& playerId, const std::string& username);
    Result<Player> getPlayer(const std::string& playerId) const;
    Result<Player> getPlayerByUsername(const std::string& username) const;
    Result<void> renamePlayer(const std::string& playerId, const std::string& newUsername);
    bool hasPlayer(const std::string& playerId) const;

    // Multi-Window Score Processing & Idempotency
    Result<ScoreUpdateResult> submitScore(const ScoreEvent& event);

    // Board Access
    std::shared_ptr<Leaderboard> getBoard(const std::string& category, TimeWindow window, timestamp_t timestamp = 0);
    std::shared_ptr<Leaderboard> getBoardById(const std::string& boardId) const;

    // Seasonal Rollover & Archive
    Result<void> resetSeason(const std::string& seasonName, bool hardReset = true);
    const std::vector<SeasonStandings>& seasonArchives() const { return seasonArchives_; }

    // Time window key helpers (UTC)
    static std::string getDailyKey(timestamp_t ms);
    static std::string getWeeklyKey(timestamp_t ms);
    static std::string getMonthlyKey(timestamp_t ms);

    size_t totalPlayers() const;
    size_t totalBoards() const;

private:
    std::shared_ptr<IClock> clock_;
    mutable std::shared_mutex managerLock_;

    // Players: ID map + case-insensitive username index
    std::unordered_map<std::string, Player> players_;
    std::unordered_map<std::string, std::string> usernameToIdMap_;

    // Boards: boardId -> Leaderboard
    std::unordered_map<std::string, std::shared_ptr<Leaderboard>> boards_;

    // Idempotency tracking: eventId -> ScoreEvent
    std::unordered_map<std::string, ScoreEvent> processedEvents_;
    std::unordered_map<std::string, ScoreUpdateResult> cachedResults_;

    // Seasonal archive records
    std::vector<SeasonStandings> seasonArchives_;

    std::string makeBoardId(const std::string& category, TimeWindow window, timestamp_t ms) const;
};

} // namespace LeaderboardEngine

#endif // LEADERBOARD_MANAGER_H
