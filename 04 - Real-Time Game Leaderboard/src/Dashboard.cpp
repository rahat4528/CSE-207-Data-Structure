#include "Dashboard.h"
#include <sstream>
#include <iomanip>

namespace LeaderboardEngine {

TerminalDashboard::TerminalDashboard(std::shared_ptr<LeaderboardManager> mgr, std::shared_ptr<ICacheAdapter> cache)
    : mgr_(std::move(mgr)), cache_(std::move(cache)) {}

std::string TerminalDashboard::renderBoxHeader(const std::string& title) const {
    std::ostringstream oss;
    oss << "========================================================================\n";
    oss << "  " << title << "\n";
    oss << "========================================================================\n";
    return oss.str();
}

std::string TerminalDashboard::renderBoardView(const std::string& category, TimeWindow window, size_t topN) const {
    auto board = mgr_->getBoard(category, window);
    auto topList = board->getTopK(topN);

    std::ostringstream oss;
    oss << renderBoxHeader("REAL-TIME GAME LEADERBOARD :: " + category + " [" + timeWindowToString(window) + "]");
    oss << " Board Version: v" << board->version() << " | Total Registered: " << board->playerCount() << "\n";
    oss << "------------------------------------------------------------------------\n";
    oss << " " << std::left << std::setw(6) << "RANK"
        << std::setw(20) << "PLAYER ID"
        << std::setw(15) << "SCORE"
        << std::setw(15) << "PERCENTILE" << "\n";
    oss << "------------------------------------------------------------------------\n";

    if (topList.empty()) {
        oss << " (No players currently ranked on this board)\n";
    } else {
        for (const auto& entry : topList) {
            std::string rankStr = "#" + std::to_string(entry.rank);
            oss << " " << std::left << std::setw(6) << rankStr
                << std::setw(20) << entry.playerId
                << std::setw(15) << entry.score
                << std::fixed << std::setprecision(1) << entry.percentile << "%\n";
        }
    }
    oss << "========================================================================\n";
    return oss.str();
}

std::string TerminalDashboard::renderPlayerCard(const std::string& playerId, const std::string& category) const {
    auto pRes = mgr_->getPlayer(playerId);
    if (!pRes.ok) {
        return "Player [" + playerId + "] not found in registry.\n";
    }

    auto board = mgr_->getBoard(category, TimeWindow::ALL_TIME);
    auto rRes = board->getRank(playerId);
    auto nextRes = board->getPointsToNextRank(playerId);
    auto pctRes = board->getPercentile(playerId);

    const Player& p = pRes.value;

    std::ostringstream oss;
    oss << renderBoxHeader("PLAYER PROFILE :: " + p.username() + " (" + playerId + ")");
    oss << " Current Score : " << p.currentScore() << "\n";
    oss << " Personal Best : " << p.bestScore() << "\n";
    oss << " Total Updates : " << p.updateCount() << "\n";
    if (rRes.ok) {
        oss << " Global Rank   : #" << rRes.value << " (Top "
            << std::fixed << std::setprecision(1) << (100.0 - pctRes.value) << "%)\n";
        if (nextRes.ok && nextRes.value > 0) {
            oss << " Next Rank In  : " << nextRes.value << " points\n";
        } else {
            oss << " Standing      : Currently Rank #1 (Champion)\n";
        }
    }
    oss << " 5-Game Trend  : " << std::fixed << std::setprecision(1) << p.calculateMovingAverage(5) << " avg pts\n";
    oss << "========================================================================\n";
    return oss.str();
}

std::string TerminalDashboard::renderSystemHealth() const {
    std::ostringstream oss;
    oss << renderBoxHeader("SYSTEM METRICS & CACHE STATUS");
    oss << " Registered Players : " << mgr_->totalPlayers() << "\n";
    oss << " Active Leaderboards: " << mgr_->totalBoards() << "\n";
    if (cache_) {
        size_t total = cache_->hitCount() + cache_->missCount();
        double hitRate = (total > 0) ? (static_cast<double>(cache_->hitCount()) / total * 100.0) : 0.0;
        oss << " Redis/Memory Cache : " << (cache_->isHealthy() ? "ONLINE" : "OFFLINE") << "\n";
        oss << " Cache Hits / Misses: " << cache_->hitCount() << " / " << cache_->missCount()
            << " (" << std::fixed << std::setprecision(1) << hitRate << "% hit rate)\n";
    }
    oss << "========================================================================\n";
    return oss.str();
}

} // namespace LeaderboardEngine
