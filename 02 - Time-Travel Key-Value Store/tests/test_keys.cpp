#include "MiniTest.h"
#include "KeyManager.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestCreateAndReadCurrentKey) {
    KeyManager km;
    ASSERT_FALSE(km.hasKey("player:100"));

    // 1. Create a new key (OP_SET)
    auto prepRes = km.prepareWrite("player:100", "Alice", 1000000ULL);
    ASSERT_TRUE(prepRes.isOk());
    VersionRecord v1 = prepRes.value();
    ASSERT_EQ(v1.globalVersionId(), 1ULL);
    ASSERT_EQ(v1.keyVersionNumber(), 1u);
    ASSERT_EQ(v1.key(), std::string("player:100"));
    ASSERT_EQ(v1.value(), std::string("Alice"));
    ASSERT_EQ(v1.opType(), OpType::OP_SET);
    ASSERT_EQ(v1.prevVersionId(), 0ULL);
    ASSERT_FALSE(v1.isDeleted());

    ASSERT_TRUE(km.applyRecord(v1).isOk());
    ASSERT_TRUE(km.hasKey("player:100"));
    ASSERT_FALSE(km.isDeleted("player:100"));

    const KeyRecord* rec = km.getKeyRecord("player:100");
    ASSERT_TRUE(rec != nullptr);
    ASSERT_EQ(rec->latestVersionNumber, 1u);
    ASSERT_EQ(rec->latestGlobalVersionId, 1ULL);
    ASSERT_EQ(rec->versions.size(), 1u);
    ASSERT_EQ(rec->versions[0].value(), std::string("Alice"));
}

TEST_CASE(TestUpdateKeyNewVersion) {
    KeyManager km;
    auto v1 = km.prepareWrite("username", "Alice", 1000ULL).value();
    km.applyRecord(v1);

    // 2. Update existing key (OP_UPDATE)
    auto prep2 = km.prepareWrite("username", "Bob", 2000ULL);
    ASSERT_TRUE(prep2.isOk());
    VersionRecord v2 = prep2.value();
    ASSERT_EQ(v2.globalVersionId(), 2ULL);
    ASSERT_EQ(v2.keyVersionNumber(), 2u);
    ASSERT_EQ(v2.prevVersionId(), 1ULL); // Linked to v1
    ASSERT_EQ(v2.value(), std::string("Bob"));
    ASSERT_EQ(v2.opType(), OpType::OP_UPDATE);

    km.applyRecord(v2);

    const KeyRecord* rec = km.getKeyRecord("username");
    ASSERT_TRUE(rec != nullptr);
    ASSERT_EQ(rec->latestVersionNumber, 2u);
    ASSERT_EQ(rec->versions.size(), 2u);
    ASSERT_EQ(rec->versions[1].value(), std::string("Bob"));
}

TEST_CASE(TestDeleteKeyCreatesTombstone) {
    KeyManager km;
    auto v1 = km.prepareWrite("session:token", "abc123xyz", 1000ULL).value();
    km.applyRecord(v1);

    // 3. Delete key (OP_DELETE)
    auto delPrep = km.prepareDelete("session:token", 2000ULL);
    ASSERT_TRUE(delPrep.isOk());
    VersionRecord v2 = delPrep.value();
    ASSERT_EQ(v2.globalVersionId(), 2ULL);
    ASSERT_EQ(v2.keyVersionNumber(), 2u);
    ASSERT_EQ(v2.opType(), OpType::OP_DELETE);
    ASSERT_TRUE(v2.isDeleted());
    ASSERT_EQ(v2.value(), std::string(""));
    ASSERT_EQ(v2.prevVersionId(), 1ULL);

    km.applyRecord(v2);

    ASSERT_TRUE(km.hasKey("session:token"));
    ASSERT_TRUE(km.isDeleted("session:token"));

    const KeyRecord* rec = km.getKeyRecord("session:token");
    ASSERT_TRUE(rec != nullptr);
    ASSERT_EQ(rec->versions.size(), 2u);
    ASSERT_EQ(rec->versions[0].value(), std::string("abc123xyz"));
    ASSERT_TRUE(rec->versions[1].isDeleted());

    // Deleting an already deleted key returns error
    auto delAgain = km.prepareDelete("session:token", 3000ULL);
    ASSERT_TRUE(delAgain.isError());
    ASSERT_EQ(delAgain.error(), ErrorCode::ERR_KEY_DELETED);
}

TEST_CASE(TestRecreateDeletedKey) {
    KeyManager km;
    auto v1 = km.prepareWrite("account", "basic", 1000ULL).value();
    km.applyRecord(v1);

    auto v2 = km.prepareDelete("account", 2000ULL).value();
    km.applyRecord(v2);
    ASSERT_TRUE(km.isDeleted("account"));

    // 4. Recreate key after deletion
    auto v3 = km.prepareWrite("account", "premium", 3000ULL).value();
    ASSERT_EQ(v3.keyVersionNumber(), 3u);
    ASSERT_EQ(v3.prevVersionId(), 2ULL); // Linked to tombstone v2
    ASSERT_EQ(v3.value(), std::string("premium"));
    ASSERT_FALSE(v3.isDeleted());

    km.applyRecord(v3);
    ASSERT_FALSE(km.isDeleted("account"));

    const KeyRecord* rec = km.getKeyRecord("account");
    ASSERT_EQ(rec->versions.size(), 3u);
    ASSERT_EQ(rec->versions[0].value(), std::string("basic"));
    ASSERT_TRUE(rec->versions[1].isDeleted());
    ASSERT_EQ(rec->versions[2].value(), std::string("premium"));
}

TEST_CASE(TestRejectDuplicateVersionIdAndBrokenChain) {
    KeyManager km;
    auto v1 = km.prepareWrite("k1", "v1", 1000ULL).value();
    ASSERT_TRUE(km.applyRecord(v1).isOk());

    // Applying duplicate version ID fails via HashSet
    auto dupRes = km.applyRecord(v1);
    ASSERT_TRUE(dupRes.isError());
    ASSERT_EQ(dupRes.error(), ErrorCode::ERR_DUPLICATE_VERSION_ID);

    // Applying broken chain (wrong prevVersionId) fails
    VersionRecord broken(99, 2, 1, "k1", "bad", OpType::OP_UPDATE, 2000ULL, 888ULL, false);
    auto brokenRes = km.applyRecord(broken);
    ASSERT_TRUE(brokenRes.isError());
    ASSERT_EQ(brokenRes.error(), ErrorCode::ERR_CORRUPTED_DATA);
}
