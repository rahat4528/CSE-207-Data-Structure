#include "TestRunner.h"
#include "LeaderboardManager.h"

using namespace LeaderboardEngine;

TEST_CASE(test_player_registration_and_uniqueness) {
    auto clock = std::make_shared<FakeClock>(100000);
    LeaderboardManager mgr(clock);

    auto res1 = mgr.registerPlayer("p1", "ShadowSniper");
    ASSERT_TRUE(res1.ok);

    // Duplicate ID rejected
    auto res2 = mgr.registerPlayer("p1", "AnotherName");
    ASSERT_FALSE(res2.ok);
    ASSERT_EQ(res2.error, ErrorCode::DUPLICATE_PLAYER_ID);

    // Duplicate case-insensitive username rejected
    auto res3 = mgr.registerPlayer("p2", "shadowsniper");
    ASSERT_FALSE(res3.ok);
    ASSERT_EQ(res3.error, ErrorCode::DUPLICATE_USERNAME);

    // Case-insensitive lookup works
    auto pLookup = mgr.getPlayerByUsername("SHADOWSNIPER");
    ASSERT_TRUE(pLookup.ok);
    ASSERT_EQ(pLookup.value.id(), "p1");
}

TEST_CASE(test_idempotency_and_conflict_detection) {
    auto clock = std::make_shared<FakeClock>(100000);
    LeaderboardManager mgr(clock);
    mgr.registerPlayer("p1", "player1");

    ScoreEvent ev1{"evt_001", "p1", "arcade", 150, ScoreUpdateMode::SET, 1000};
    auto res1 = mgr.submitScore(ev1);
    ASSERT_TRUE(res1.ok);
    ASSERT_EQ(res1.value.newScore, 150);

    // Exact same event ID and payload returns cached result
    auto res2 = mgr.submitScore(ev1);
    ASSERT_TRUE(res2.ok);
    ASSERT_EQ(res2.value.newScore, 150);

    // Same event ID with differing score/payload returns 409 Conflict
    ScoreEvent evConflict{"evt_001", "p1", "arcade", 999, ScoreUpdateMode::SET, 1000};
    auto res3 = mgr.submitScore(evConflict);
    ASSERT_FALSE(res3.ok);
    ASSERT_EQ(res3.error, ErrorCode::EVENT_PAYLOAD_CONFLICT);
}

TEST_CASE(test_temporal_windows_and_clock_rollover) {
    // 2026-11-08 23:59:59 UTC -> 1794182399000 ms approx
    timestamp_t t1 = 1794182399000LL;
    auto clock = std::make_shared<FakeClock>(t1);
    LeaderboardManager mgr(clock);

    mgr.registerPlayer("p1", "night_warrior");

    // Submit on day 1
    mgr.submitScore({"e1", "p1", "pvp", 100, ScoreUpdateMode::SET, t1});

    std::string dayKey1 = LeaderboardManager::getDailyKey(t1);
    auto daily1 = mgr.getBoard("pvp", TimeWindow::DAILY, t1);
    ASSERT_EQ(daily1->getRank("p1").value, 1);
    ASSERT_EQ(daily1->playerCount(), 1);

    // Advance clock 2 seconds -> next calendar day 00:00:01 UTC
    timestamp_t t2 = t1 + 2000;
    clock->setTimeMs(t2);

    std::string dayKey2 = LeaderboardManager::getDailyKey(t2);
    ASSERT_NE(dayKey1, dayKey2);

    auto daily2 = mgr.getBoard("pvp", TimeWindow::DAILY, t2);
    // On the new day, board is fresh with 0 players until new score arrives
    ASSERT_EQ(daily2->playerCount(), 0);

    // All-time board still retains p1
    auto allTime = mgr.getBoard("pvp", TimeWindow::ALL_TIME, t2);
    ASSERT_EQ(allTime->playerCount(), 1);
    ASSERT_EQ(allTime->getRank("p1").value, 1);
}

TEST_CASE(test_season_reset_and_archive) {
    auto clock = std::make_shared<FakeClock>(100000);
    LeaderboardManager mgr(clock);

    mgr.registerPlayer("p1", "champion");
    mgr.submitScore({"e1", "p1", "ranked", 5000, ScoreUpdateMode::SET, 1000});

    auto rankedBoard = mgr.getBoard("ranked", TimeWindow::ALL_TIME);
    ASSERT_EQ(rankedBoard->getTopK(1)[0].score, 5000);

    // Reset Season 1
    mgr.resetSeason("Season_2026_Q4", true);

    ASSERT_EQ(mgr.seasonArchives().size(), mgr.totalBoards());
    ASSERT_EQ(mgr.seasonArchives()[0].seasonName, "Season_2026_Q4");

    // Board has been reset to 0 players
    auto freshBoard = mgr.getBoard("ranked", TimeWindow::ALL_TIME);
    ASSERT_EQ(freshBoard->playerCount(), 0);
}
