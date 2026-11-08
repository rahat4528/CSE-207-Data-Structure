#include "TestRunner.h"
#include "Persistence.h"
#include <cstdio>

using namespace LeaderboardEngine;

TEST_CASE(test_wal_and_recovery_roundtrip) {
    auto clock = std::make_shared<FakeClock>(1000);
    auto mgr1 = std::make_shared<LeaderboardManager>(clock);
    auto audit1 = std::make_shared<AuditManager>(mgr1);

    std::string testDir = "./";
    PersistenceManager pm1(testDir, mgr1, audit1);

    // Register and submit scores
    mgr1->registerPlayer("p1", "Alice");
    pm1.appendPlayerRegistration("p1", "Alice", 1000);

    mgr1->registerPlayer("p2", "Bob");
    pm1.appendPlayerRegistration("p2", "Bob", 1000);

    ScoreEvent ev1{"e1", "p1", "arcade", 250, ScoreUpdateMode::SET, 1000};
    mgr1->submitScore(ev1);
    pm1.appendScoreEvent(ev1);

    ScoreEvent ev2{"e2", "p2", "arcade", 500, ScoreUpdateMode::SET, 1000};
    mgr1->submitScore(ev2);
    pm1.appendScoreEvent(ev2);

    pm1.close();

    // Now start a fresh manager and recover from disk
    auto mgr2 = std::make_shared<LeaderboardManager>(clock);
    auto audit2 = std::make_shared<AuditManager>(mgr2);
    PersistenceManager pm2(testDir, mgr2, audit2);

    auto recRes = pm2.recoverFromDisk("leaderboard.snap", "events.wal");
    ASSERT_TRUE(recRes.ok);
    ASSERT_EQ(recRes.value, 4); // 2 registrations + 2 score events

    // Verify state restored
    ASSERT_EQ(mgr2->totalPlayers(), 2);
    auto board = mgr2->getBoard("arcade", TimeWindow::ALL_TIME);
    ASSERT_EQ(board->getRank("p2").value, 1); // 500 pts
    ASSERT_EQ(board->getRank("p1").value, 2); // 250 pts
    ASSERT_TRUE(board->validateInvariant());

    pm2.close();
    std::remove("events.wal");
}
