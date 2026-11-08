#include "LeaderboardManager.h"
#include <ctime>
#include <cstdio>

namespace LeaderboardEngine {

LeaderboardManager::LeaderboardManager(std::shared_ptr<IClock> clock)
    : clock_(std::move(clock)) {}

std::string LeaderboardManager::getDailyKey(timestamp_t ms) {
    time_t sec = static_cast<time_t>(ms / 1000);
    struct tm tmUtc;
#if defined(_WIN32)
    gmtime_s(&tmUtc, &sec);
#else
    gmtime_r(&sec, &tmUtc);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tmUtc.tm_year + 1900, tmUtc.tm_mon + 1, tmUtc.tm_mday);
    return std::string(buf);
}

std::string LeaderboardManager::getMonthlyKey(timestamp_t ms) {
    time_t sec = static_cast<time_t>(ms / 1000);
    struct tm tmUtc;
#if defined(_WIN32)
    gmtime_s(&tmUtc, &sec);
#else
    gmtime_r(&sec, &tmUtc);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d", tmUtc.tm_year + 1900, tmUtc.tm_mon + 1);
    return std::string(buf);
}

std::string LeaderboardManager::getWeeklyKey(timestamp_t ms) {
    time_t sec = static_cast<time_t>(ms / 1000);
    struct tm tmUtc;
#if defined(_WIN32)
    gmtime_s(&tmUtc, &sec);
#else
    gmtime_r(&sec, &tmUtc);
#endif
    char buf[32];
    // Approximation of year and week number
    int weekNum = (tmUtc.tm_yday / 7) + 1;
    std::snprintf(buf, sizeof(buf), "%04d-W%02d", tmUtc.tm_year + 1900, weekNum);
    return std::string(buf);
}

std::string LeaderboardManager::makeBoardId(const std::string& category, TimeWindow window, timestamp_t ms) const {
    if (window == TimeWindow::ALL_TIME) {
        return category + ":all_time";
    } else if (window == TimeWindow::DAILY) {
        return category + ":daily:" + getDailyKey(ms);
    } else if (window == TimeWindow::WEEKLY) {
        return category + ":weekly:" + getWeeklyKey(ms);
    } else if (window == TimeWindow::MONTHLY) {
        return category + ":monthly:" + getMonthlyKey(ms);
    }
    return category + ":custom";
}

Result<void> LeaderboardManager::registerPlayer(const std::string& playerId, const std::string& username) {
    std::unique_lock<std::shared_mutex> lock(managerLock_);

    if (!isValidPlayerId(playerId)) {
        return Result<void>::Failure(ErrorCode::INVALID_PLAYER_ID, "Invalid player ID format or length");
    }
    if (!isValidUsername(username)) {
        return Result<void>::Failure(ErrorCode::INVALID_USERNAME, "Invalid username format or length");
    }

    if (players_.find(playerId) != players_.end()) {
        return Result<void>::Failure(ErrorCode::DUPLICATE_PLAYER_ID, "Player ID already registered");
    }

    std::string lowerUser = toLower(username);
    if (usernameToIdMap_.find(lowerUser) != usernameToIdMap_.end()) {
        return Result<void>::Failure(ErrorCode::DUPLICATE_USERNAME, "Username already taken (case-insensitive)");
    }

    timestamp_t now = clock_->nowMs();
    players_[playerId] = Player(playerId, username, now);
    usernameToIdMap_[lowerUser] = playerId;

    return Result<void>::Success();
}

Result<Player> LeaderboardManager::getPlayer(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(managerLock_);
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return Result<Player>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not found");
    }
    return Result<Player>::Success(it->second);
}

Result<Player> LeaderboardManager::getPlayerByUsername(const std::string& username) const {
    std::shared_lock<std::shared_mutex> lock(managerLock_);
    std::string lowerUser = toLower(username);
    auto it = usernameToIdMap_.find(lowerUser);
    if (it == usernameToIdMap_.end()) {
        return Result<Player>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player username not found");
    }
    return getPlayer(it->second);
}

Result<void> LeaderboardManager::renamePlayer(const std::string& playerId, const std::string& newUsername) {
    std::unique_lock<std::shared_mutex> lock(managerLock_);
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        return Result<void>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not found");
    }
    if (!isValidUsername(newUsername)) {
        return Result<void>::Failure(ErrorCode::INVALID_USERNAME, "Invalid username format");
    }

    std::string lowerNew = toLower(newUsername);
    auto uIt = usernameToIdMap_.find(lowerNew);
    if (uIt != usernameToIdMap_.end() && uIt->second != playerId) {
        return Result<void>::Failure(ErrorCode::DUPLICATE_USERNAME, "Username already taken");
    }

    std::string oldLower = toLower(it->second.username());
    usernameToIdMap_.erase(oldLower);
    usernameToIdMap_[lowerNew] = playerId;
    it->second.setUsername(newUsername);

    return Result<void>::Success();
}

bool LeaderboardManager::hasPlayer(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(managerLock_);
    return players_.find(playerId) != players_.end();
}

std::shared_ptr<Leaderboard> LeaderboardManager::getBoard(const std::string& category, TimeWindow window, timestamp_t timestamp) {
    std::unique_lock<std::shared_mutex> lock(managerLock_);
    if (timestamp == 0) {
        timestamp = clock_->nowMs();
    }
    std::string bId = makeBoardId(category, window, timestamp);
    auto it = boards_.find(bId);
    if (it != boards_.end()) {
        return it->second;
    }

    auto newBoard = std::make_shared<Leaderboard>(bId, category, window);
    boards_[bId] = newBoard;
    return newBoard;
}

std::shared_ptr<Leaderboard> LeaderboardManager::getBoardById(const std::string& boardId) const {
    std::shared_lock<std::shared_mutex> lock(managerLock_);
    auto it = boards_.find(boardId);
    if (it != boards_.end()) {
        return it->second;
    }
    return nullptr;
}

Result<ScoreUpdateResult> LeaderboardManager::submitScore(const ScoreEvent& event) {
    std::unique_lock<std::shared_mutex> lock(managerLock_);

    // 1. Verify player is registered
    auto pIt = players_.find(event.playerId);
    if (pIt == players_.end()) {
        return Result<ScoreUpdateResult>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not registered");
    }

    // 2. Validate Event ID
    if (event.eventId.empty()) {
        return Result<ScoreUpdateResult>::Failure(ErrorCode::DUPLICATE_EVENT_ID, "Event ID cannot be empty");
    }

    // 3. Idempotency Check
    auto eIt = processedEvents_.find(event.eventId);
    if (eIt != processedEvents_.end()) {
        if (eIt->second == event) {
            // Replay cached result
            return Result<ScoreUpdateResult>::Success(cachedResults_[event.eventId]);
        } else {
            return Result<ScoreUpdateResult>::Failure(ErrorCode::EVENT_PAYLOAD_CONFLICT,
                "Event ID already processed with different payload (409 Conflict)");
        }
    }

    // 4. Update Category Active Window Boards (All-Time, Daily, Weekly, Monthly)
    timestamp_t eventTime = (event.timestamp > 0) ? event.timestamp : clock_->nowMs();

    std::string allTimeId = makeBoardId(event.category, TimeWindow::ALL_TIME, eventTime);
    std::string dailyId = makeBoardId(event.category, TimeWindow::DAILY, eventTime);
    std::string weeklyId = makeBoardId(event.category, TimeWindow::WEEKLY, eventTime);
    std::string monthlyId = makeBoardId(event.category, TimeWindow::MONTHLY, eventTime);

    // Ensure boards exist
    auto getOrCreate = [&](const std::string& id, TimeWindow win) {
        auto it = boards_.find(id);
        if (it == boards_.end()) {
            auto b = std::make_shared<Leaderboard>(id, event.category, win);
            boards_[id] = b;
            return b;
        }
        return it->second;
    };

    auto allTimeBoard = getOrCreate(allTimeId, TimeWindow::ALL_TIME);
    auto dailyBoard = getOrCreate(dailyId, TimeWindow::DAILY);
    auto weeklyBoard = getOrCreate(weeklyId, TimeWindow::WEEKLY);
    auto monthlyBoard = getOrCreate(monthlyId, TimeWindow::MONTHLY);

    // Apply score update
    auto primaryResult = allTimeBoard->updateScore(event);
    if (!primaryResult.ok) {
        return primaryResult;
    }

    dailyBoard->updateScore(event);
    weeklyBoard->updateScore(event);
    monthlyBoard->updateScore(event);

    // 5. Update player overall statistics
    pIt->second.recordScoreUpdate(primaryResult.value.newScore, eventTime, event.eventId);
    pIt->second.recordRankSnapshot(primaryResult.value.newRank, eventTime);

    // 6. Cache for idempotency
    processedEvents_[event.eventId] = event;
    cachedResults_[event.eventId] = primaryResult.value;

    return primaryResult;
}

Result<void> LeaderboardManager::resetSeason(const std::string& seasonName, bool hardReset) {
    std::unique_lock<std::shared_mutex> lock(managerLock_);

    timestamp_t now = clock_->nowMs();
    for (auto& kv : boards_) {
        SeasonStandings archive;
        archive.seasonName = seasonName;
        archive.archivedAt = now;
        archive.topEntries = kv.second->getTopK(100);
        seasonArchives_.push_back(std::move(archive));

        if (hardReset) {
            // Re-instantiate boards fresh
            kv.second = std::make_shared<Leaderboard>(kv.second->boardId(), kv.second->category(), kv.second->window());
        }
    }

    return Result<void>::Success();
}

size_t LeaderboardManager::totalPlayers() const {
    std::shared_lock<std::shared_mutex> lock(managerLock_);
    return players_.size();
}

size_t LeaderboardManager::totalBoards() const {
    std::shared_lock<std::shared_mutex> lock(managerLock_);
    return boards_.size();
}

} // namespace LeaderboardEngine
