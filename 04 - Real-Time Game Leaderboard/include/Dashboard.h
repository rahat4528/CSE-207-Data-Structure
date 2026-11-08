#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "Common.h"
#include "Leaderboard.h"
#include "LeaderboardManager.h"
#include "CacheAdapter.h"
#include <string>
#include <vector>

namespace LeaderboardEngine {

class TerminalDashboard {
public:
    TerminalDashboard(std::shared_ptr<LeaderboardManager> mgr, std::shared_ptr<ICacheAdapter> cache);

    std::string renderBoardView(const std::string& category, TimeWindow window, size_t topN = 10) const;
    std::string renderPlayerCard(const std::string& playerId, const std::string& category) const;
    std::string renderSystemHealth() const;

private:
    std::shared_ptr<LeaderboardManager> mgr_;
    std::shared_ptr<ICacheAdapter> cache_;

    std::string renderBoxHeader(const std::string& title) const;
};

} // namespace LeaderboardEngine

#endif // DASHBOARD_H
