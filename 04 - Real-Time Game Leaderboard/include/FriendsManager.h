#ifndef FRIENDS_MANAGER_H
#define FRIENDS_MANAGER_H

#include "Common.h"
#include "Leaderboard.h"
#include "LeaderboardManager.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>

namespace LeaderboardEngine {

struct PlayerComparison {
    std::string playerA;
    std::string playerB;
    score_t scoreA{0};
    score_t scoreB{0};
    size_t rankA{0};
    size_t rankB{0};
    score_t scoreDiff{0};
    int64_t rankDiff{0};
    score_t pointsToOvertake{0};
    std::string leader;
};

class FriendsManager {
public:
    FriendsManager(std::shared_ptr<LeaderboardManager> mgr);

    Result<void> addFriend(const std::string& playerId, const std::string& friendId);
    Result<void> removeFriend(const std::string& playerId, const std::string& friendId);
    bool areFriends(const std::string& p1, const std::string& p2) const;
    std::vector<std::string> getFriends(const std::string& playerId) const;

    // Generates a dedicated friends leaderboard ranking in O(F log F)
    std::vector<LeaderboardEntry> getFriendsLeaderboard(const std::string& playerId, const std::string& category, TimeWindow window);

    // Direct head-to-head comparison
    Result<PlayerComparison> comparePlayers(const std::string& p1, const std::string& p2, const std::string& category);

private:
    std::shared_ptr<LeaderboardManager> mgr_;
    mutable std::shared_mutex friendsLock_;

    // Adjacency set of friendships
    std::unordered_map<std::string, std::unordered_set<std::string>> friendsMap_;
};

} // namespace LeaderboardEngine

#endif // FRIENDS_MANAGER_H
