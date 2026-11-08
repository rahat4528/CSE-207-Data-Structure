#include "MiniTest.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "SnapshotEngine.h"
#include "TemporalSearch.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestRollbackCreatesNewVersionPreservesHistory) {
    KeyManager km;

    // Timeline for "config":
    // v1 (t=1000): "debug=true"
    // v2 (t=2000): "debug=false"
    // v3 (t=3000): "debug=verbose"
    km.applyRecord(km.prepareWrite("config", "debug=true", 1000ULL).value());
    km.applyRecord(km.prepareWrite("config", "debug=false", 2000ULL).value());
    km.applyRecord(km.prepareWrite("config", "debug=verbose", 3000ULL).value());

    const KeyRecord* rec = km.getKeyRecord("config");
    ASSERT_EQ(rec->latestVersionNumber, 3u);

    // Roll back to version 1 at t=4000
    auto rbRes = SnapshotEngine::rollbackKeyToVersion(km, "config", 1, 4000ULL, "ops_team");
    ASSERT_TRUE(rbRes.isOk());
    VersionRecord v4 = rbRes.value();

    // Verify v4 is a NEW version with restored value
    ASSERT_EQ(v4.keyVersionNumber(), 4u);
    ASSERT_EQ(v4.opType(), OpType::OP_ROLLBACK);
    ASSERT_EQ(v4.value(), std::string("debug=true"));
    ASSERT_EQ(v4.prevVersionId(), 3ULL); // Linked to v3

    km.applyRecord(v4);

    // Intervening versions 2 and 3 are still fully queryable
    ASSERT_EQ(rec->versions.size(), 4u);
    ASSERT_EQ(TemporalSearch::queryByVersionNumber(*rec, 2).value, std::string("debug=false"));
    ASSERT_EQ(TemporalSearch::queryByVersionNumber(*rec, 3).value, std::string("debug=verbose"));
    ASSERT_EQ(TemporalSearch::queryByVersionNumber(*rec, 4).value, std::string("debug=true"));
}

TEST_CASE(TestNamedSnapshotCreateAndRestore) {
    KeyManager km;
    SnapshotEngine se;

    // t=1000: Initial state for v1.0 release
    km.applyRecord(km.prepareWrite("feature:darkmode", "off", 1000ULL).value());
    km.applyRecord(km.prepareWrite("feature:ai_chat", "disabled", 1000ULL).value());

    // Create named snapshot "release-1.0"
    ASSERT_TRUE(se.createNamedSnapshot("release-1.0", 1000ULL, 2ULL, "release_mgr", "Release 1.0 baseline").isOk());
    ASSERT_TRUE(se.hasNamedSnapshot("release-1.0"));

    // Mutate state in development (t=2000):
    km.applyRecord(km.prepareWrite("feature:darkmode", "on", 2000ULL).value());
    km.applyRecord(km.prepareWrite("feature:ai_chat", "enabled", 2000ULL).value());
    km.applyRecord(km.prepareWrite("feature:beta_tool", "experimental", 2000ULL).value());

    ASSERT_EQ(km.keyCount(false), 3u);

    // Now restore named snapshot "release-1.0" at t=3000
    auto restoreRes = se.restoreNamedSnapshot(km, "release-1.0", 3000ULL, "emergency_rollback");
    ASSERT_TRUE(restoreRes.isOk());
    ASSERT_EQ(restoreRes.value().size(), 3u); // 2 restored, 1 deleted

    for (const auto& rec : restoreRes.value()) {
        km.applyRecord(rec);
    }

    // After restore: darkmode is "off", ai_chat is "disabled", beta_tool is DELETED
    const KeyRecord* kDark = km.getKeyRecord("feature:darkmode");
    const KeyRecord* kChat = km.getKeyRecord("feature:ai_chat");
    const KeyRecord* kBeta = km.getKeyRecord("feature:beta_tool");

    ASSERT_EQ(TemporalSearch::queryCurrent(*kDark).value, std::string("off"));
    ASSERT_EQ(TemporalSearch::queryCurrent(*kChat).value, std::string("disabled"));
    ASSERT_TRUE(TemporalSearch::queryCurrent(*kBeta).isDeleted);

    // Dropping snapshot
    ASSERT_TRUE(se.dropNamedSnapshot("release-1.0").isOk());
    ASSERT_FALSE(se.hasNamedSnapshot("release-1.0"));
}

TEST_CASE(TestTTLExpirationInstantAndSweep) {
    KeyManager km;

    // Set key "session:auth" with expiry at t=2000ULL
    auto v1 = km.prepareWrite("session:auth", "token_abc", 1000ULL, 2000ULL).value();
    km.applyRecord(v1);

    const KeyRecord* rec = km.getKeyRecord("session:auth");
    ASSERT_TRUE(rec != nullptr);

    // 1. Query at t=1500 (before expiry): FOUND
    auto q1500 = TemporalSearch::queryByTimestamp(*rec, 1500ULL);
    ASSERT_EQ(q1500.status, QueryStatus::FOUND);
    ASSERT_EQ(q1500.value, std::string("token_abc"));

    // 2. Query at t=2500 (after expiry): EXPIRED (instant evaluation without physical sweep)
    auto q2500 = TemporalSearch::queryByTimestamp(*rec, 2500ULL);
    ASSERT_EQ(q2500.status, QueryStatus::EXPIRED);

    // 3. Active sweep at t=3000ULL: generates OP_EXPIRE tombstone
    auto swept = SnapshotEngine::sweepExpiredKeys(km, 3000ULL);
    ASSERT_EQ(swept.size(), 1u);
    ASSERT_EQ(swept[0].opType(), OpType::OP_EXPIRE);
    ASSERT_TRUE(swept[0].isDeleted());

    km.applyRecord(swept[0]);
    ASSERT_TRUE(km.isDeleted("session:auth"));

    // Query current on swept key returns DELETED
    ASSERT_EQ(TemporalSearch::queryCurrent(*rec).status, QueryStatus::DELETED);
    // Historical query at t=1500 still returns "token_abc"
    ASSERT_EQ(TemporalSearch::queryByTimestamp(*rec, 1500ULL).value, std::string("token_abc"));
}
