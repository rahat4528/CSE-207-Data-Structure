#include "CLI.h"
#include <iostream>
#include <sstream>

namespace LeaderboardEngine {

CLI::CLI(std::shared_ptr<LeaderboardManager> mgr,
         std::shared_ptr<FriendsManager> friends,
         std::shared_ptr<AuditManager> audit,
         std::shared_ptr<ICacheAdapter> cache)
    : mgr_(std::move(mgr)), friends_(std::move(friends)), audit_(std::move(audit)),
      cache_(std::move(cache)), dashboard_(mgr_, cache_) {}

void CLI::printHelp() const {
    std::cout << "\n========================================================================\n"
              << "  REAL-TIME GAME LEADERBOARD :: COMMAND LINE UTILITY\n"
              << "========================================================================\n"
              << "Commands:\n"
              << "  register <id> <username>              Register new player\n"
              << "  score <id> <score> [cat] [mode]       Submit score (mode: SET/ADD/MAX)\n"
              << "  rank <id> [cat]                       Query exact rank and stats\n"
              << "  top [k] [cat]                         Display Top K players (default: 10)\n"
              << "  page <offset> <limit> [cat]           Paginate leaderboard entries\n"
              << "  around <id> [radius] [cat]            Show players near given player rank\n"
              << "  player <id> [cat]                     Display player profile and trend\n"
              << "  friend add <p1> <p2>                  Establish mutual friendship\n"
              << "  friends <id> [cat]                    Display friends-only leaderboard\n"
              << "  compare <p1> <p2> [cat]               Head-to-head ranking comparison\n"
              << "  correct <token> <id> <score> <reason> Administrative score adjustment\n"
              << "  audit                                 Display administrative audit log\n"
              << "  health                                Show system metrics and cache stats\n"
              << "  seed                                  Populate seed players and scores\n"
              << "  repl                                  Enter interactive shell\n"
              << "  help                                  Show this help message\n"
              << "========================================================================\n\n";
}

void CLI::seedDemoData() {
    std::cout << "[Seed] Registering players...\n";
    mgr_->registerPlayer("p1", "ShadowSniper");
    mgr_->registerPlayer("p2", "Valkyrie");
    mgr_->registerPlayer("p3", "GhostRider");
    mgr_->registerPlayer("p4", "IronFist");
    mgr_->registerPlayer("p5", "NeonBlade");
    mgr_->registerPlayer("p6", "CyberNinja");

    std::cout << "[Seed] Submitting competitive scores for 'arcade'...\n";
    mgr_->submitScore({"s1", "p1", "arcade", 12500, ScoreUpdateMode::SET, 1000});
    mgr_->submitScore({"s2", "p2", "arcade", 14200, ScoreUpdateMode::SET, 1010});
    mgr_->submitScore({"s3", "p3", "arcade", 9800,  ScoreUpdateMode::SET, 1020});
    mgr_->submitScore({"s4", "p4", "arcade", 14200, ScoreUpdateMode::SET, 1030}); // Tie with Valkyrie
    mgr_->submitScore({"s5", "p5", "arcade", 11000, ScoreUpdateMode::SET, 1040});
    mgr_->submitScore({"s6", "p6", "arcade", 7600,  ScoreUpdateMode::SET, 1050});

    std::cout << "[Seed] Setting up friendships...\n";
    friends_->addFriend("p1", "p2");
    friends_->addFriend("p1", "p3");
    friends_->addFriend("p1", "p5");

    std::cout << "[Seed] Done! Demo players and scores initialized.\n";
}

int CLI::executeCommand(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "help") {
        printHelp();
        return 0;
    }

    const std::string& cmd = args[0];

    if (cmd == "register" && args.size() >= 3) {
        auto res = mgr_->registerPlayer(args[1], args[2]);
        if (res.ok) {
            std::cout << "SUCCESS: Player [" << args[1] << "] registered with username @" << args[2] << "\n";
            return 0;
        } else {
            std::cerr << "ERROR: " << res.message << " (" << res.error << ")\n";
            return 1;
        }
    } else if (cmd == "score" && args.size() >= 3) {
        std::string cat = (args.size() >= 4) ? args[3] : "arcade";
        ScoreUpdateMode mode = ScoreUpdateMode::SET;
        if (args.size() >= 5) {
            if (args[4] == "ADD") mode = ScoreUpdateMode::ADD;
            else if (args[4] == "MAX") mode = ScoreUpdateMode::MAX;
        }
        score_t val = std::stoll(args[2]);
        std::string eid = "cli_ev_" + args[1] + "_" + std::to_string(std::time(nullptr));
        auto res = mgr_->submitScore({eid, args[1], cat, val, mode, 0});
        if (res.ok) {
            std::cout << "SUCCESS: Player [" << args[1] << "] score updated: "
                      << res.value.oldScore << " -> " << res.value.newScore
                      << " | Rank: #" << res.value.newRank << "\n";
            return 0;
        } else {
            std::cerr << "ERROR: " << res.message << "\n";
            return 1;
        }
    } else if (cmd == "rank" && args.size() >= 2) {
        std::string cat = (args.size() >= 3) ? args[2] : "arcade";
        auto board = mgr_->getBoard(cat, TimeWindow::ALL_TIME);
        auto r = board->getRank(args[1]);
        if (r.ok) {
            std::cout << "Player [" << args[1] << "] Rank in '" << cat << "': #" << r.value << "\n";
            return 0;
        } else {
            std::cerr << "ERROR: " << r.message << "\n";
            return 1;
        }
    } else if (cmd == "top") {
        size_t k = (args.size() >= 2) ? std::stoul(args[1]) : 10;
        std::string cat = (args.size() >= 3) ? args[2] : "arcade";
        std::cout << dashboard_.renderBoardView(cat, TimeWindow::ALL_TIME, k);
        return 0;
    } else if (cmd == "page" && args.size() >= 3) {
        size_t offset = std::stoul(args[1]);
        size_t limit = std::stoul(args[2]);
        std::string cat = (args.size() >= 4) ? args[3] : "arcade";
        auto board = mgr_->getBoard(cat, TimeWindow::ALL_TIME);
        auto page = board->getPage(offset, limit);
        std::cout << "Leaderboard Page (offset: " << offset << ", limit: " << limit << "):\n";
        for (const auto& item : page) {
            std::cout << "  #" << item.rank << " " << item.playerId << " - " << item.score << " pts\n";
        }
        return 0;
    } else if (cmd == "around" && args.size() >= 2) {
        size_t radius = (args.size() >= 3) ? std::stoul(args[2]) : 2;
        std::string cat = (args.size() >= 4) ? args[3] : "arcade";
        auto board = mgr_->getBoard(cat, TimeWindow::ALL_TIME);
        auto around = board->getPlayersAround(args[1], radius);
        std::cout << "Players Around [" << args[1] << "]:\n";
        for (const auto& item : around) {
            std::cout << "  #" << item.rank << " " << item.playerId << " - " << item.score << " pts"
                      << (item.playerId == args[1] ? " <== (YOU)" : "") << "\n";
        }
        return 0;
    } else if (cmd == "player" && args.size() >= 2) {
        std::string cat = (args.size() >= 3) ? args[2] : "arcade";
        std::cout << dashboard_.renderPlayerCard(args[1], cat);
        return 0;
    } else if (cmd == "friends" && args.size() >= 2) {
        std::string cat = (args.size() >= 3) ? args[2] : "arcade";
        auto fEntries = friends_->getFriendsLeaderboard(args[1], cat, TimeWindow::ALL_TIME);
        std::cout << "\n=======================================================\n"
                  << "  FRIENDS LEADERBOARD :: " << args[1] << "\n"
                  << "=======================================================\n";
        for (const auto& f : fEntries) {
            std::cout << "  #" << f.rank << " " << f.playerId << " : " << f.score << " pts"
                      << (f.playerId == args[1] ? " (YOU)" : "") << "\n";
        }
        std::cout << "=======================================================\n\n";
        return 0;
    } else if (cmd == "compare" && args.size() >= 3) {
        std::string cat = (args.size() >= 4) ? args[3] : "arcade";
        auto compRes = friends_->comparePlayers(args[1], args[2], cat);
        if (compRes.ok) {
            const auto& c = compRes.value;
            std::cout << "\n=======================================================\n"
                      << "  HEAD-TO-HEAD COMPARISON: " << c.playerA << " vs " << c.playerB << "\n"
                      << "=======================================================\n"
                      << " " << c.playerA << " : " << c.scoreA << " pts (#" << c.rankA << ")\n"
                      << " " << c.playerB << " : " << c.scoreB << " pts (#" << c.rankB << ")\n"
                      << " Score Lead : " << c.leader << " by " << c.scoreDiff << " pts\n"
                      << " Overtake In: " << c.pointsToOvertake << " points\n"
                      << "=======================================================\n\n";
            return 0;
        } else {
            std::cerr << "ERROR: " << compRes.message << "\n";
            return 1;
        }
    } else if (cmd == "correct" && args.size() >= 5) {
        AdminCorrectionRequest req{
            "admin_root",
            args[1],
            args[2],
            "arcade",
            std::stoll(args[3]),
            args[4]
        };
        auto res = audit_->applyCorrection(req);
        if (res.ok) {
            std::cout << "SUCCESS: Admin correction applied to player [" << args[2] << "].\n";
            return 0;
        } else {
            std::cerr << "ERROR: " << res.message << "\n";
            return 1;
        }
    } else if (cmd == "audit") {
        std::cout << "\n=======================================================\n"
                  << "  ADMINISTRATIVE AUDIT LOG (" << audit_->auditCount() << " entries)\n"
                  << "=======================================================\n";
        for (const auto& a : audit_->auditLog()) {
            std::cout << " [" << a.auditId << "] Admin: " << a.adminId
                      << " | Player: " << a.playerId << " | " << a.oldScore << " -> " << a.newScore
                      << " | Reason: " << a.reason << "\n";
        }
        std::cout << "=======================================================\n\n";
        return 0;
    } else if (cmd == "health") {
        std::cout << dashboard_.renderSystemHealth();
        return 0;
    } else if (cmd == "seed") {
        seedDemoData();
        return 0;
    } else if (cmd == "repl") {
        runRepl();
        return 0;
    }

    std::cerr << "Unknown command. Run 'help' to see usage.\n";
    return 1;
}

void CLI::runRepl() {
    std::cout << "\n========================================================================\n"
              << "  REAL-TIME GAME LEADERBOARD INTERACTIVE REPL (Type 'exit' to quit)\n"
              << "========================================================================\n";
    std::string line;
    while (true) {
        std::cout << "leaderboard> ";
        if (!std::getline(std::cin, line) || line == "exit" || line == "quit") {
            std::cout << "Exiting REPL...\n";
            break;
        }
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::vector<std::string> args;
        std::string token;
        while (iss >> token) {
            args.push_back(token);
        }
        executeCommand(args);
    }
}

} // namespace LeaderboardEngine
