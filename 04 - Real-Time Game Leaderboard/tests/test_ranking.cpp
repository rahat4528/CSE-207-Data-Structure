#include "TestRunner.h"
#include "Leaderboard.h"
#include <random>

using namespace LeaderboardEngine;

TEST_CASE(test_competition_ranking_canonical_example) {
    Leaderboard board("arcade:all_time", "arcade", TimeWindow::ALL_TIME);

    // Worked example from Section 6.2: Scores 100, 90, 90, 80
    // Must produce competition ranks: 1, 2, 2, 4
    board.updateScore({"e1", "alice", "arcade", 100, ScoreUpdateMode::SET, 1000});
    board.updateScore({"e2", "bob", "arcade", 90, ScoreUpdateMode::SET, 1010});
    board.updateScore({"e3", "charlie", "arcade", 90, ScoreUpdateMode::SET, 1020});
    board.updateScore({"e4", "david", "arcade", 80, ScoreUpdateMode::SET, 1030});

    ASSERT_EQ(board.getRank("alice").value, 1);
    ASSERT_EQ(board.getRank("bob").value, 2);
    ASSERT_EQ(board.getRank("charlie").value, 2); // Equal scores share competition rank 2
    ASSERT_EQ(board.getRank("david").value, 4);   // Next rank skips to 4 (1224 competition rank)

    ASSERT_EQ(board.getDenseRank(100).value, 1);
    ASSERT_EQ(board.getDenseRank(90).value, 2);
    ASSERT_EQ(board.getDenseRank(80).value, 3); // Dense rank: 1, 2, 3

    ASSERT_TRUE(board.validateInvariant());
}

TEST_CASE(test_score_update_modes) {
    Leaderboard board("rpg:all_time", "rpg", TimeWindow::ALL_TIME);

    // SET mode
    board.updateScore({"e1", "hero", "rpg", 100, ScoreUpdateMode::SET, 1000});
    ASSERT_EQ(board.getRank("hero").value, 1);

    // ADD mode
    board.updateScore({"e2", "hero", "rpg", 50, ScoreUpdateMode::ADD, 1010}); // 100 + 50 = 150
    auto top1 = board.getTopK(1);
    ASSERT_EQ(top1[0].score, 150);

    // MAX mode (higher replaces)
    board.updateScore({"e3", "hero", "rpg", 200, ScoreUpdateMode::MAX, 1020}); // 200 > 150 -> 200
    ASSERT_EQ(board.getTopK(1)[0].score, 200);

    // MAX mode (lower ignored)
    board.updateScore({"e4", "hero", "rpg", 180, ScoreUpdateMode::MAX, 1030}); // 180 <= 200 -> still 200
    ASSERT_EQ(board.getTopK(1)[0].score, 200);
}

TEST_CASE(test_display_tie_breaking_order) {
    Leaderboard board("racing:all_time", "racing", TimeWindow::ALL_TIME);

    // Equal scores:
    // "p1" at t=1000
    // "p2" at t=900 (earlier timestamp -> wins tie-break)
    // "p3" at t=1000 (lexical comparison with p1 -> "p1" < "p3")
    board.updateScore({"e1", "p1", "racing", 500, ScoreUpdateMode::SET, 1000});
    board.updateScore({"e2", "p2", "racing", 500, ScoreUpdateMode::SET, 900});
    board.updateScore({"e3", "p3", "racing", 500, ScoreUpdateMode::SET, 1000});

    auto topList = board.getTopK(3);
    ASSERT_EQ(topList.size(), 3);
    ASSERT_EQ(topList[0].playerId, "p2"); // Earlier timestamp (900 < 1000)
    ASSERT_EQ(topList[1].playerId, "p1"); // Tie on timestamp -> lexical ID ("p1" < "p3")
    ASSERT_EQ(topList[2].playerId, "p3");
}

TEST_CASE(test_points_to_next_rank_and_percentiles) {
    Leaderboard board("puzzle:all_time", "puzzle", TimeWindow::ALL_TIME);

    board.updateScore({"e1", "p1", "puzzle", 100, ScoreUpdateMode::SET, 1000});
    board.updateScore({"e2", "p2", "puzzle", 80, ScoreUpdateMode::SET, 1010});
    board.updateScore({"e3", "p3", "puzzle", 50, ScoreUpdateMode::SET, 1020});

    // p1 is rank 1 -> 0 points to next rank
    ASSERT_EQ(board.getPointsToNextRank("p1").value, 0);

    // p2 is rank 2 (80 pts) -> needs 100 - 80 = 20 points to overtake rank 1
    ASSERT_EQ(board.getPointsToNextRank("p2").value, 20);

    // p3 is rank 3 (50 pts) -> needs 80 - 50 = 30 points to overtake rank 2
    ASSERT_EQ(board.getPointsToNextRank("p3").value, 30);

    // Percentiles
    // p1: (3-1)/3 * 100 = 66.67%
    // p3: (3-3)/3 * 100 = 0.0%
    ASSERT_NEAR(board.getPercentile("p1").value, 66.666, 0.1);
    ASSERT_NEAR(board.getPercentile("p3").value, 0.0, 0.1);
}

TEST_CASE(test_randomized_invariants_and_removals) {
    Leaderboard board("stress:all_time", "stress", TimeWindow::ALL_TIME);
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> scoreDist(10, 500);

    // Insert 50 players
    for (int i = 1; i <= 50; ++i) {
        std::string pid = "player_" + std::to_string(i);
        board.updateScore({"e_" + pid, pid, "stress", scoreDist(rng), ScoreUpdateMode::SET, 1000 + i});
    }

    ASSERT_TRUE(board.validateInvariant());

    // Update 30 random players
    for (int i = 1; i <= 30; ++i) {
        std::string pid = "player_" + std::to_string((i * 7) % 50 + 1);
        board.updateScore({"upd_" + pid, pid, "stress", scoreDist(rng), ScoreUpdateMode::SET, 2000 + i});
    }

    ASSERT_TRUE(board.validateInvariant());

    // Remove 10 players
    for (int i = 1; i <= 10; ++i) {
        std::string pid = "player_" + std::to_string(i);
        board.removePlayer(pid);
    }

    ASSERT_EQ(board.playerCount(), 40);
    ASSERT_TRUE(board.validateInvariant());
}
