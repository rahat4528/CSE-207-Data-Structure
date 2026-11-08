#include "MiniTest.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "AuditLogger.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestEventLogAppendAndReplayStateIdentity) {
    KeyManager kmOriginal;
    EventLog el;

    // Execute a series of writes on kmOriginal and append to EventLog
    auto v1 = kmOriginal.prepareWrite("user:1", "Alice", 1000ULL, 0, "admin", "create user").value();
    kmOriginal.applyRecord(v1);
    el.appendEvent(v1);

    auto v2 = kmOriginal.prepareWrite("user:2", "Bob", 1100ULL, 0, "admin", "create user 2").value();
    kmOriginal.applyRecord(v2);
    el.appendEvent(v2);

    auto v3 = kmOriginal.prepareWrite("user:1", "Alice_Updated", 1200ULL, 0, "admin", "update user").value();
    kmOriginal.applyRecord(v3);
    el.appendEvent(v3);

    auto v4 = kmOriginal.prepareDelete("user:2", 1300ULL, "admin", "delete user 2").value();
    kmOriginal.applyRecord(v4);
    el.appendEvent(v4);

    ASSERT_EQ(el.count(), 4u);

    // Replay entire EventLog into a brand new, empty KeyManager
    KeyManager kmReplayed;
    auto replayRes = el.replayInto(kmReplayed);
    ASSERT_TRUE(replayRes.isOk());

    // Verify exact identity between live kmOriginal and kmReplayed
    ASSERT_EQ(kmReplayed.keyCount(true), kmOriginal.keyCount(true));
    ASSERT_EQ(kmReplayed.totalVersionCount(), kmOriginal.totalVersionCount());
    ASSERT_EQ(kmReplayed.isDeleted("user:2"), kmOriginal.isDeleted("user:2"));

    const KeyRecord* orig1 = kmOriginal.getKeyRecord("user:1");
    const KeyRecord* repl1 = kmReplayed.getKeyRecord("user:1");
    ASSERT_TRUE(orig1 != nullptr && repl1 != nullptr);
    ASSERT_EQ(repl1->versions.size(), orig1->versions.size());
    for (size_t i = 0; i < orig1->versions.size(); ++i) {
        ASSERT_EQ(repl1->versions[i].globalVersionId(), orig1->versions[i].globalVersionId());
        ASSERT_EQ(repl1->versions[i].keyVersionNumber(), orig1->versions[i].keyVersionNumber());
        ASSERT_EQ(repl1->versions[i].value(), orig1->versions[i].value());
        ASSERT_EQ(repl1->versions[i].timestamp(), orig1->versions[i].timestamp());
    }
}

TEST_CASE(TestAuditPolicyFiltering) {
    AuditPolicy policy;
    policy.logReads = false;   // READs disabled by default
    policy.logWrites = true;
    policy.logDeletes = true;

    AuditLogger logger(policy);

    // Log a read -> should NOT be recorded
    logger.log(1000ULL, "READ", "profile", "guest", true);
    ASSERT_EQ(logger.count(), 0u);

    // Log a write -> should be recorded
    logger.log(1100ULL, "WRITE", "profile", "admin", true, ErrorCode::OK, "val updated");
    ASSERT_EQ(logger.count(), 1u);
    ASSERT_EQ(logger.records()[0].operation, std::string("WRITE"));
    ASSERT_EQ(logger.records()[0].key, std::string("profile"));

    // Enable read auditing
    policy.logReads = true;
    logger.setPolicy(policy);

    logger.log(1200ULL, "READ", "profile", "guest", true);
    ASSERT_EQ(logger.count(), 2u);
    ASSERT_EQ(logger.records()[1].operation, std::string("READ"));

    // Filter query by operation
    auto writeOnly = logger.query("WRITE");
    ASSERT_EQ(writeOnly.size(), 1u);
    ASSERT_EQ(writeOnly[0].key, std::string("profile"));

    // Filter query by non-matching key
    auto nonMatching = logger.query(std::nullopt, "non_existent_key");
    ASSERT_EQ(nonMatching.size(), 0u);
}
