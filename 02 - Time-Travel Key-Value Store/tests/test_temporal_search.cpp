#include "MiniTest.h"
#include "TemporalSearch.h"
#include "KeyManager.h"
#include <random>
#include <string>
#include <vector>

using namespace ttkv;

namespace {
// Brute-force linear scan ORACLE: exists strictly for test validation
int linearScanOracle(const std::vector<VersionRecord>& versions, uint64_t targetTime) {
    int best = -1;
    for (size_t i = 0; i < versions.size(); ++i) {
        if (versions[i].timestamp() <= targetTime) {
            best = static_cast<int>(i);
        }
    }
    return best;
}
} // namespace

TEST_CASE(TestPointInTimeQueriesBasic) {
    KeyRecord kr;
    kr.key = "profile";
    kr.keyId = 1;

    // t=1000: v1 "Alice"
    kr.versions.push_back(VersionRecord(1, 1, 1, "profile", "Alice", OpType::OP_SET, 1000ULL, 0ULL, false));
    // t=2000: v2 "Bob"
    kr.versions.push_back(VersionRecord(2, 2, 1, "profile", "Bob", OpType::OP_UPDATE, 2000ULL, 1ULL, false));
    // t=3000: v3 Tombstone
    kr.versions.push_back(VersionRecord(3, 3, 1, "profile", "", OpType::OP_DELETE, 3000ULL, 2ULL, true));
    // t=4000: v4 "Charlie"
    kr.versions.push_back(VersionRecord(4, 4, 1, "profile", "Charlie", OpType::OP_SET, 4000ULL, 3ULL, false));

    kr.latestVersionNumber = 4;
    kr.latestGlobalVersionId = 4;

    // 1. Query by version number
    auto qv1 = TemporalSearch::queryByVersionNumber(kr, 1);
    ASSERT_EQ(qv1.status, QueryStatus::FOUND);
    ASSERT_EQ(qv1.value, std::string("Alice"));

    auto qv2 = TemporalSearch::queryByVersionNumber(kr, 2);
    ASSERT_EQ(qv2.status, QueryStatus::FOUND);
    ASSERT_EQ(qv2.value, std::string("Bob"));

    auto qv3 = TemporalSearch::queryByVersionNumber(kr, 3);
    ASSERT_EQ(qv3.status, QueryStatus::DELETED);

    auto qv4 = TemporalSearch::queryByVersionNumber(kr, 4);
    ASSERT_EQ(qv4.status, QueryStatus::FOUND);
    ASSERT_EQ(qv4.value, std::string("Charlie"));

    // Invalid version numbers
    ASSERT_EQ(TemporalSearch::queryByVersionNumber(kr, 0).status, QueryStatus::INVALID_VERSION);
    ASSERT_EQ(TemporalSearch::queryByVersionNumber(kr, 5).status, QueryStatus::INVALID_VERSION);

    // 2. Query by timestamp
    // Prior to first version (t=500) -> NOT_YET_EXISTS
    ASSERT_EQ(TemporalSearch::queryByTimestamp(kr, 500ULL).status, QueryStatus::NOT_YET_EXISTS);

    // Exactly at t=1000 -> "Alice"
    auto qt1000 = TemporalSearch::queryByTimestamp(kr, 1000ULL);
    ASSERT_EQ(qt1000.status, QueryStatus::FOUND);
    ASSERT_EQ(qt1000.value, std::string("Alice"));

    // Between t=1000 and t=2000 (t=1500) -> "Alice"
    auto qt1500 = TemporalSearch::queryByTimestamp(kr, 1500ULL);
    ASSERT_EQ(qt1500.status, QueryStatus::FOUND);
    ASSERT_EQ(qt1500.value, std::string("Alice"));

    // At t=2500 -> "Bob"
    auto qt2500 = TemporalSearch::queryByTimestamp(kr, 2500ULL);
    ASSERT_EQ(qt2500.status, QueryStatus::FOUND);
    ASSERT_EQ(qt2500.value, std::string("Bob"));

    // At t=3500 (during tombstone) -> DELETED
    auto qt3500 = TemporalSearch::queryByTimestamp(kr, 3500ULL);
    ASSERT_EQ(qt3500.status, QueryStatus::DELETED);

    // After latest version (t=5000) -> "Charlie"
    auto qt5000 = TemporalSearch::queryByTimestamp(kr, 5000ULL);
    ASSERT_EQ(qt5000.status, QueryStatus::FOUND);
    ASSERT_EQ(qt5000.value, std::string("Charlie"));
}

TEST_CASE(TestIdenticalTimestampsOrdering) {
    KeyRecord kr;
    kr.key = "counter";
    kr.keyId = 2;

    // Three versions committed at the exact same timestamp (t=1000)
    kr.versions.push_back(VersionRecord(10, 1, 2, "counter", "1", OpType::OP_SET, 1000ULL, 0ULL, false));
    kr.versions.push_back(VersionRecord(11, 2, 2, "counter", "2", OpType::OP_UPDATE, 1000ULL, 10ULL, false));
    kr.versions.push_back(VersionRecord(12, 3, 2, "counter", "3", OpType::OP_UPDATE, 1000ULL, 11ULL, false));

    kr.latestVersionNumber = 3;
    kr.latestGlobalVersionId = 12;

    // Upper-bound binary search must resolve to the latest version at t=1000 (which is version 3 with value "3")
    auto res = TemporalSearch::queryByTimestamp(kr, 1000ULL);
    ASSERT_EQ(res.status, QueryStatus::FOUND);
    ASSERT_EQ(res.versionNumber, 3u);
    ASSERT_EQ(res.globalVersionId, 12ULL);
    ASSERT_EQ(res.value, std::string("3"));
}

TEST_CASE(TestBinarySearchOraclePropertyValidation) {
    // Randomized property-based test: generate random histories of lengths up to 500,
    // with identical and non-identical timestamps, and verify binary search against linearScanOracle
    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<uint64_t> timeDeltaDist(0, 50); // Some 0-deltas (identical timestamps)
    std::uniform_int_distribution<uint64_t> queryDist(0, 100000);

    for (int run = 0; run < 100; ++run) {
        std::vector<VersionRecord> versions;
        size_t count = static_cast<size_t>(rng() % 300) + 1; // 1 to 300 versions
        uint64_t currTime = 1000ULL;
        uint64_t prevId = 0;

        for (size_t i = 0; i < count; ++i) {
            currTime += timeDeltaDist(rng);
            uint64_t gId = i + 1;
            uint32_t kVer = static_cast<uint32_t>(i + 1);
            versions.push_back(VersionRecord(
                gId, kVer, 1, "testKey", "val_" + std::to_string(i),
                OpType::OP_UPDATE, currTime, prevId, false
            ));
            prevId = gId;
        }

        // Test boundary timestamps
        ASSERT_EQ(TemporalSearch::binarySearchUpperBound(versions, 0ULL),
                  linearScanOracle(versions, 0ULL));
        ASSERT_EQ(TemporalSearch::binarySearchUpperBound(versions, versions.front().timestamp()),
                  linearScanOracle(versions, versions.front().timestamp()));
        ASSERT_EQ(TemporalSearch::binarySearchUpperBound(versions, versions.back().timestamp()),
                  linearScanOracle(versions, versions.back().timestamp()));
        ASSERT_EQ(TemporalSearch::binarySearchUpperBound(versions, versions.back().timestamp() + 5000ULL),
                  linearScanOracle(versions, versions.back().timestamp() + 5000ULL));

        // Test random query timestamps across the history range
        for (int q = 0; q < 50; ++q) {
            uint64_t target = queryDist(rng);
            int bsResult = TemporalSearch::binarySearchUpperBound(versions, target);
            int oracleResult = linearScanOracle(versions, target);
            ASSERT_EQ(bsResult, oracleResult);
        }
    }
}
