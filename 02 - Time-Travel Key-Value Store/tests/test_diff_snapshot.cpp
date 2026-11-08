#include "MiniTest.h"
#include "KeyManager.h"
#include "SnapshotEngine.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestSingleKeyDiff) {
    KeyManager km;

    // t=1000: v1 "v1"
    auto v1 = km.prepareWrite("status", "pending", 1000ULL).value();
    km.applyRecord(v1);

    // t=2000: v2 "in_progress"
    auto v2 = km.prepareWrite("status", "in_progress", 2000ULL).value();
    km.applyRecord(v2);

    // t=3000: v3 "completed"
    auto v3 = km.prepareWrite("status", "completed", 3000ULL).value();
    km.applyRecord(v3);

    const KeyRecord* rec = km.getKeyRecord("status");
    ASSERT_TRUE(rec != nullptr);

    // Diff between t=1000 and t=2500 -> MODIFIED ("pending" -> "in_progress")
    auto d1 = SnapshotEngine::diffKey(*rec, 1000ULL, 2500ULL);
    ASSERT_TRUE(d1.isOk());
    ASSERT_EQ(d1.value().changeType, std::string("MODIFIED"));
    ASSERT_EQ(d1.value().stateA.value, std::string("pending"));
    ASSERT_EQ(d1.value().stateB.value, std::string("in_progress"));

    // Diff between t=2000 and t=2500 -> UNCHANGED ("in_progress" == "in_progress")
    auto d2 = SnapshotEngine::diffKey(*rec, 2000ULL, 2500ULL);
    ASSERT_TRUE(d2.isOk());
    ASSERT_EQ(d2.value().changeType, std::string("UNCHANGED"));

    // Reversed times -> ERR_INVALID_TIMESTAMP
    auto dRev = SnapshotEngine::diffKey(*rec, 3000ULL, 1000ULL);
    ASSERT_TRUE(dRev.isError());
    ASSERT_EQ(dRev.error(), ErrorCode::ERR_INVALID_TIMESTAMP);
}

TEST_CASE(TestStoreDiffAndSnapshotTime) {
    KeyManager km;

    // Timeline:
    // t=1000: key1 = "A"
    // t=1200: key2 = "B"
    // t=2000: key1 = "A_modified", key3 = "C"
    // t=3000: key2 is DELETED
    km.applyRecord(km.prepareWrite("key1", "A", 1000ULL).value());
    km.applyRecord(km.prepareWrite("key2", "B", 1200ULL).value());
    km.applyRecord(km.prepareWrite("key1", "A_modified", 2000ULL).value());
    km.applyRecord(km.prepareWrite("key3", "C", 2000ULL).value());
    km.applyRecord(km.prepareDelete("key2", 3000ULL).value());

    // Snapshot at t=1500: should have key1="A" and key2="B"
    auto snap1500 = SnapshotEngine::takeTimeSnapshot(km, 1500ULL);
    ASSERT_EQ(snap1500.activeEntries.size(), 2u);

    bool foundK1 = false, foundK2 = false;
    for (const auto& entry : snap1500.activeEntries) {
        if (entry.key == "key1") {
            foundK1 = true;
            ASSERT_EQ(entry.value, std::string("A"));
        } else if (entry.key == "key2") {
            foundK2 = true;
            ASSERT_EQ(entry.value, std::string("B"));
        }
    }
    ASSERT_TRUE(foundK1 && foundK2);

    // Snapshot at t=3500: key1="A_modified", key2 deleted (omitted), key3="C"
    auto snap3500 = SnapshotEngine::takeTimeSnapshot(km, 3500ULL);
    ASSERT_EQ(snap3500.activeEntries.size(), 2u);

    // Store Diff between t=1500 and t=3500:
    // added: key3
    // modified: key1
    // deleted: key2
    auto storeDiffRes = SnapshotEngine::diffStore(km, 1500ULL, 3500ULL);
    ASSERT_TRUE(storeDiffRes.isOk());
    const auto& sd = storeDiffRes.value();

    ASSERT_EQ(sd.added.size(), 1u);
    ASSERT_EQ(sd.added[0].key, std::string("key3"));

    ASSERT_EQ(sd.modified.size(), 1u);
    ASSERT_EQ(sd.modified[0].key, std::string("key1"));
    ASSERT_EQ(sd.modified[0].stateA.value, std::string("A"));
    ASSERT_EQ(sd.modified[0].stateB.value, std::string("A_modified"));

    ASSERT_EQ(sd.deleted.size(), 1u);
    ASSERT_EQ(sd.deleted[0].key, std::string("key2"));
}

TEST_CASE(TestKeySearchByNameAndHistory) {
    KeyManager km;
    km.applyRecord(km.prepareWrite("user:alice", "admin", 1000ULL).value());
    km.applyRecord(km.prepareWrite("user:bob", "guest", 1100ULL).value());
    km.applyRecord(km.prepareWrite("session:123", "active", 1200ULL).value());
    km.applyRecord(km.prepareDelete("user:bob", 2000ULL).value());

    // Search prefix "user:" active only
    auto curUserKeys = SnapshotEngine::searchKeys(km, "user:", false);
    ASSERT_EQ(curUserKeys.size(), 1u);
    ASSERT_EQ(curUserKeys[0], std::string("user:alice"));

    // Search prefix "user:" including deleted
    auto allUserKeys = SnapshotEngine::searchKeys(km, "user:", true);
    ASSERT_EQ(allUserKeys.size(), 2u);

    // Search prefix "user:" as of t=1500 (before bob was deleted)
    auto histUserKeys = SnapshotEngine::searchKeys(km, "user:", false, 1500ULL);
    ASSERT_EQ(histUserKeys.size(), 2u);
}
