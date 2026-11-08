#include "FriendsManager.h"
#include <algorithm>

namespace LeaderboardEngine {

FriendsManager::FriendsManager(std::shared_ptr<LeaderboardManager> mgr)
    : mgr_(std::move(mgr)) {}

Result<void> FriendsManager::addFriend(const std::string& playerId, const std::string& friendId) {
    if (playerId == friendId) {
        return Result<void>::Failure(ErrorCode::CANNOT_FRIEND_SELF, "Cannot add yourself as a friend");
    }

    if (!mgr_->hasPlayer(playerId) || !mgr_->hasPlayer(friendId)) {
        return Result<void>::Failure(ErrorCode::PLAYER_NOT_FOUND, "One or both players do not exist");
    }

    std::unique_lock<std::shared_mutex> lock(friendsLock_);
    if (friendsMap_[playerId].count(friendId) > 0) {
        return Result<void>::Failure(ErrorCode::ALREADY_FRIENDS, "Players are already friends");
    }

    friendsMap_[playerId].insert(friendId);
    friendsMap_[friendId].insert(playerId); // Mutual friendship

    return Result<void>::Success();
}

Result<void> FriendsManager::removeFriend(const std::string& playerId, const std::string& friendId) {
    std::unique_lock<std::shared_mutex> lock(friendsLock_);
    auto it = friendsMap_.find(playerId);
    if (it == friendsMap_.end() || it->second.count(friendId) == 0) {
        return Result<void>::Failure(ErrorCode::NOT_FRIENDS, "Not friends");
    }

    friendsMap_[playerId].erase(friendId);
    friendsMap_[friendId].erase(playerId);

    return Result<void>::Success();
}

bool FriendsManager::areFriends(const std::string& p1, const std::string& p2) const {
    std::shared_lock<std::shared_mutex> lock(friendsLock_);
    auto it = friendsMap_.find(p1);
    if (it == friendsMap_.end()) return false;
    return it->second.count(p2) > 0;
}

std::vector<std::string> FriendsManager::getFriends(const std::string& playerId) const {
    std::shared_lock<std::shared_mutex> lock(friendsLock_);
    std::vector<std::string> list;
    auto it = friendsMap_.find(playerId);
    if (it != friendsMap_.end()) {
        list.assign(it->second.begin(), it->second.end());
    }
    return list;
}

std::vector<LeaderboardEntry> FriendsManager::getFriendsLeaderboard(const std::string& playerId, const std::string& category, TimeWindow window) {
    std::vector<std::string> friendGroup;
    {
        std::shared_lock<std::shared_mutex> lock(friendsLock_);
        friendGroup.push_back(playerId);
        auto it = friendsMap_.find(playerId);
        if (it != friendsMap_.end()) {
            for (const auto& f : it->second) {
                friendGroup.push_back(f);
            }
        }
    }

    auto board = mgr_->getBoard(category, window);
    std::vector<LeaderboardEntry> entries;
    entries.reserve(friendGroup.size());

    for (const auto& pid : friendGroup) {
        auto pRes = mgr_->getPlayer(pid);
        if (pRes.ok) {
            auto rankRes = board->getRank(pid);
            size_t r = rankRes.ok ? rankRes.value : 0;
            entries.push_back({r, pid, pRes.value.currentScore(), pRes.value.lastUpdateTime(), 0.0});
        }
    }

    // Sort in strict descending score order with tie-breaking
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
        return a.playerId < b.playerId;
    });

    // Compute relative competition rank in friends group ("1224")
    for (size_t i = 0; i < entries.size(); ++i) {
        size_t rank = 1;
        for (size_t j = 0; j < entries.size(); ++j) {
            if (entries[j].score > entries[i].score) {
                rank++;
            }
        }
        entries[i].rank = rank;
    }

    return entries;
}

Result<PlayerComparison> FriendsManager::comparePlayers(const std::string& p1, const std::string& p2, const std::string& category) {
    auto board = mgr_->getBoard(category, TimeWindow::ALL_TIME);
    auto resP1 = mgr_->getPlayer(p1);
    auto resP2 = mgr_->getPlayer(p2);

    if (!resP1.ok || !resP2.ok) {
        return Result<PlayerComparison>::Failure(ErrorCode::PLAYER_NOT_FOUND, "One or both players not found");
    }

    auto r1 = board->getRank(p1);
    auto r2 = board->getRank(p2);

    PlayerComparison comp;
    comp.playerA = p1;
    comp.playerB = p2;
    comp.scoreA = resP1.value.currentScore();
    comp.scoreB = resP2.value.currentScore();
    comp.rankA = r1.ok ? r1.value : 0;
    comp.rankB = r2.ok ? r2.value : 0;
    comp.scoreDiff = std::abs(comp.scoreA - comp.scoreB);
    comp.rankDiff = static_cast<int64_t>(comp.rankA) - static_cast<int64_t>(comp.rankB);

    if (comp.scoreA > comp.scoreB) {
        comp.leader = p1;
        comp.pointsToOvertake = comp.scoreA - comp.scoreB + 1;
    } else if (comp.scoreB > comp.scoreA) {
        comp.leader = p2;
        comp.pointsToOvertake = comp.scoreB - comp.scoreA + 1;
    } else {
        comp.leader = "TIE";
        comp.pointsToOvertake = 1;
    }

    return Result<PlayerComparison>::Success(comp);
}

} // namespace LeaderboardEngine
