#include "MiniTest.h"
#include "KeyManager.h"
#include "TemporalSearch.h"
#include "HistoryEngine.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestTombstoneLifecycleAndQueries) {
    KeyManager km;

    // t=1000: SET key "user1" -> "Alice"
    auto v1 = km.prepareWrite("user1", "Alice", 1000ULL).value();
    km.applyRecord(v1);

    // t=2000: UPDATE key "user1" -> "Bob"
    auto v2 = km.prepareWrite("user1", "Bob", 2000ULL).value();
    km.applyRecord(v2);

    // t=3000: DELETE key "user1" -> Tombstone
    auto v3 = km.prepareDelete("user1", 3000ULL).value();
    km.applyRecord(v3);

    const KeyRecord* rec = km.getKeyRecord("user1");
    ASSERT_TRUE(rec != nullptr);

    // 1. Current query on deleted key returns DELETED status
    auto curRes = TemporalSearch::queryCurrent(*rec);
    ASSERT_EQ(curRes.status, QueryStatus::DELETED);
    ASSERT_TRUE(curRes.isDeleted);

    // 2. Query at t=1500 (before update 2) returns "Alice"
    auto q1500 = TemporalSearch::queryByTimestamp(*rec, 1500ULL);
    ASSERT_EQ(q1500.status, QueryStatus::FOUND);
    ASSERT_EQ(q1500.value, std::string("Alice"));

    // 3. Query at t=2500 (before deletion) returns "Bob"
    auto q2500 = TemporalSearch::queryByTimestamp(*rec, 2500ULL);
    ASSERT_EQ(q2500.status, QueryStatus::FOUND);
    ASSERT_EQ(q2500.value, std::string("Bob"));

    // 4. Query at t=3500 (after deletion) returns DELETED
    auto q3500 = TemporalSearch::queryByTimestamp(*rec, 3500ULL);
    ASSERT_EQ(q3500.status, QueryStatus::DELETED);

    // 5. Query version 2 directly -> "Bob"
    auto qv2 = TemporalSearch::queryByVersionNumber(*rec, 2);
    ASSERT_EQ(qv2.status, QueryStatus::FOUND);
    ASSERT_EQ(qv2.value, std::string("Bob"));

    // 6. Query version 3 directly -> DELETED
    auto qv3 = TemporalSearch::queryByVersionNumber(*rec, 3);
    ASSERT_EQ(qv3.status, QueryStatus::DELETED);

    // 7. Recreate key at t=4000 -> "Charlie"
    auto v4 = km.prepareWrite("user1", "Charlie", 4000ULL).value();
    km.applyRecord(v4);

    ASSERT_FALSE(km.isDeleted("user1"));
    auto curAfterRecreate = TemporalSearch::queryCurrent(*rec);
    ASSERT_EQ(curAfterRecreate.status, QueryStatus::FOUND);
    ASSERT_EQ(curAfterRecreate.value, std::string("Charlie"));
    ASSERT_EQ(curAfterRecreate.versionNumber, 4u);

    // All previous history is still intact!
    ASSERT_EQ(TemporalSearch::queryByTimestamp(*rec, 2500ULL).value, std::string("Bob"));
    ASSERT_EQ(TemporalSearch::queryByTimestamp(*rec, 3500ULL).status, QueryStatus::DELETED);
    ASSERT_EQ(TemporalSearch::queryByTimestamp(*rec, 4500ULL).value, std::string("Charlie"));
}

TEST_CASE(TestHistoryPaginationAndFormatting) {
    KeyManager km;

    for (int i = 1; i <= 10; ++i) {
        auto v = km.prepareWrite("doc:report", "revision_" + std::to_string(i),
                                 static_cast<uint64_t>(i * 1000)).value();
        km.applyRecord(v);
    }

    const KeyRecord* rec = km.getKeyRecord("doc:report");
    ASSERT_TRUE(rec != nullptr);

    // Full history
    auto allHist = HistoryEngine::getKeyHistory(*rec);
    ASSERT_TRUE(allHist.isOk());
    ASSERT_EQ(allHist.value().size(), 10u);

    // Pagination: limit 3, offset 2 -> versions 3, 4, 5
    auto page = HistoryEngine::getKeyHistory(*rec, 3, 2);
    ASSERT_TRUE(page.isOk());
    ASSERT_EQ(page.value().size(), 3u);
    ASSERT_EQ(page.value()[0].versionNumber, 3u);
    ASSERT_EQ(page.value()[1].versionNumber, 4u);
    ASSERT_EQ(page.value()[2].versionNumber, 5u);

    // Format table
    std::string table = HistoryEngine::formatHistoryTable("doc:report", page.value());
    ASSERT_TRUE(!table.empty());
    ASSERT_TRUE(table.find("revision_3") != std::string::npos);
}
