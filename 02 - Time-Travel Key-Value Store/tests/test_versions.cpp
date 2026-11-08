#include "MiniTest.h"
#include "KeyManager.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestBlueprintUsernameSequence) {
    KeyManager km;

    // Version 1: Alice
    auto v1 = km.prepareWrite("username", "Alice", 1000ULL, 0, "admin", "Initial user").value();
    km.applyRecord(v1);

    // Version 2: Bob
    auto v2 = km.prepareWrite("username", "Bob", 2000ULL, 0, "admin", "Name update").value();
    km.applyRecord(v2);

    // Version 3: Charlie
    auto v3 = km.prepareWrite("username", "Charlie", 3000ULL, 0, "admin", "Second update").value();
    km.applyRecord(v3);

    // Version 4: Deleted
    auto v4 = km.prepareDelete("username", 4000ULL, "admin", "User removed").value();
    km.applyRecord(v4);

    const KeyRecord* rec = km.getKeyRecord("username");
    ASSERT_TRUE(rec != nullptr);
    ASSERT_EQ(rec->versions.size(), 4u);
    ASSERT_EQ(rec->latestVersionNumber, 4u);
    ASSERT_TRUE(rec->isDeleted);

    ASSERT_EQ(rec->versions[0].keyVersionNumber(), 1u);
    ASSERT_EQ(rec->versions[0].value(), std::string("Alice"));
    ASSERT_EQ(rec->versions[0].description(), std::string("Initial user"));

    ASSERT_EQ(rec->versions[1].keyVersionNumber(), 2u);
    ASSERT_EQ(rec->versions[1].value(), std::string("Bob"));

    ASSERT_EQ(rec->versions[2].keyVersionNumber(), 3u);
    ASSERT_EQ(rec->versions[2].value(), std::string("Charlie"));

    ASSERT_EQ(rec->versions[3].keyVersionNumber(), 4u);
    ASSERT_TRUE(rec->versions[3].isDeleted());
    ASSERT_EQ(rec->versions[3].description(), std::string("User removed"));
}

TEST_CASE(TestGlobalVersusPerKeyVersionProgression) {
    KeyManager km;

    // Interleaved writes to keyA and keyB
    auto a1 = km.prepareWrite("keyA", "valA1", 1000ULL).value();
    km.applyRecord(a1); // global=1, keyA ver=1

    auto b1 = km.prepareWrite("keyB", "valB1", 1100ULL).value();
    km.applyRecord(b1); // global=2, keyB ver=1

    auto a2 = km.prepareWrite("keyA", "valA2", 1200ULL).value();
    km.applyRecord(a2); // global=3, keyA ver=2

    auto b2 = km.prepareWrite("keyB", "valB2", 1300ULL).value();
    km.applyRecord(b2); // global=4, keyB ver=2

    const KeyRecord* recA = km.getKeyRecord("keyA");
    const KeyRecord* recB = km.getKeyRecord("keyB");

    ASSERT_EQ(recA->versions[0].globalVersionId(), 1ULL);
    ASSERT_EQ(recA->versions[0].keyVersionNumber(), 1u);
    ASSERT_EQ(recA->versions[1].globalVersionId(), 3ULL);
    ASSERT_EQ(recA->versions[1].keyVersionNumber(), 2u);

    ASSERT_EQ(recB->versions[0].globalVersionId(), 2ULL);
    ASSERT_EQ(recB->versions[0].keyVersionNumber(), 1u);
    ASSERT_EQ(recB->versions[1].globalVersionId(), 4ULL);
    ASSERT_EQ(recB->versions[1].keyVersionNumber(), 2u);

    ASSERT_EQ(km.totalVersionCount(), 4u);
    ASSERT_EQ(km.keyCount(), 2u);
}
