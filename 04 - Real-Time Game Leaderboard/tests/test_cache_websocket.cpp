#include "TestRunner.h"
#include "CacheAdapter.h"
#include "WebSocketBroadcaster.h"

using namespace LeaderboardEngine;

TEST_CASE(test_cache_ttl_and_invalidation) {
    auto clock = std::make_shared<FakeClock>(1000);
    InMemoryCacheAdapter cache(clock);

    cache.set("leaderboard:arcade:top10", "serialized_data", 5); // 5 sec TTL

    std::string val;
    ASSERT_TRUE(cache.get("leaderboard:arcade:top10", val));
    ASSERT_EQ(val, "serialized_data");
    ASSERT_EQ(cache.hitCount(), 1);

    // Advance clock past TTL (6 seconds)
    clock->advanceMs(6000);
    ASSERT_FALSE(cache.get("leaderboard:arcade:top10", val));
    ASSERT_EQ(cache.missCount(), 1);

    // Prefix invalidation
    cache.set("user:1:rank", "1", 60);
    cache.set("user:2:rank", "2", 60);
    cache.set("game:config", "cfg", 60);

    cache.invalidatePrefix("user:");
    ASSERT_FALSE(cache.get("user:1:rank", val));
    ASSERT_FALSE(cache.get("user:2:rank", val));
    ASSERT_TRUE(cache.get("game:config", val));
}

TEST_CASE(test_websocket_broadcaster_events) {
    MockWebSocketBroadcaster broadcaster;
    ASSERT_EQ(broadcaster.eventCount(), 0);

    std::vector<LeaderboardEntry> topList = {
        {1, "p1", 500, 1000, 100.0},
        {2, "p2", 300, 1010, 50.0}
    };

    broadcaster.broadcastTopK("arcade:all_time", 42, topList);
    broadcaster.broadcastPlayerRank("p1", 2, 1, 500);

    auto events = broadcaster.getEvents();
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].boardVersion, 42);
    ASSERT_EQ(events[0].boardId, "arcade:all_time");
}
