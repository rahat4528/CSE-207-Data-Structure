#include "MiniTest.h"
#include "Common.h"
#include "CRC32.h"
#include "Timestamp.h"
#include "IClock.h"
#include "Validation.h"
#include <string>
#include <vector>

using namespace ttkv;

TEST_CASE(TestResultMonad) {
    auto okRes = Result<int>::ok(42);
    ASSERT_TRUE(okRes.isOk());
    ASSERT_FALSE(okRes.isError());
    ASSERT_EQ(okRes.value(), 42);
    ASSERT_EQ(okRes.error(), ErrorCode::OK);

    auto errRes = Result<int>::error(ErrorCode::ERR_KEY_UNKNOWN, "Key 'foo' not found");
    ASSERT_TRUE(errRes.isError());
    ASSERT_FALSE(errRes.isOk());
    ASSERT_EQ(errRes.error(), ErrorCode::ERR_KEY_UNKNOWN);
    ASSERT_EQ(errRes.message(), std::string("Key 'foo' not found"));

    auto voidOk = Result<void>::ok();
    ASSERT_TRUE(voidOk.isOk());

    auto voidErr = Result<void>::error(ErrorCode::ERR_CORRUPTED_DATA, "Bad CRC");
    ASSERT_TRUE(voidErr.isError());
    ASSERT_EQ(voidErr.error(), ErrorCode::ERR_CORRUPTED_DATA);
}

TEST_CASE(TestCRC32KnownVectors) {
    // Standard test vector: "123456789" -> 0xCBF43926
    std::string testStr = "123456789";
    uint32_t crc = CRC32::calculate(testStr);
    ASSERT_EQ(crc, 0xCBF43926u);

    // Empty string CRC32 is 0x00000000
    uint32_t emptyCrc = CRC32::calculate("");
    ASSERT_EQ(emptyCrc, 0x00000000u);

    // Incremental update matches single calculate
    CRC32 incremental;
    incremental.update("12345");
    incremental.update("6789");
    ASSERT_EQ(incremental.get(), 0xCBF43926u);
}

TEST_CASE(TestTimestampParsingValid) {
    // 1970-01-01T00:00:00Z -> 0 microseconds
    auto r1 = Timestamp::parseIso8601("1970-01-01T00:00:00Z");
    ASSERT_TRUE(r1.isOk());
    ASSERT_EQ(r1.value(), 0ULL);

    // 2026-11-01T10:30:00Z (as in blueprint)
    auto r2 = Timestamp::parseIso8601("2026-11-01T10:30:00Z");
    ASSERT_TRUE(r2.isOk());
    std::string formatted = Timestamp::formatIso8601(r2.value());
    ASSERT_EQ(formatted, std::string("2026-11-01T10:30:00Z"));

    // Fractional seconds: 2026-11-01T10:30:00.500000Z
    auto r3 = Timestamp::parseIso8601("2026-11-01T10:30:00.500000Z");
    ASSERT_TRUE(r3.isOk());
    ASSERT_EQ(r3.value(), r2.value() + 500000ULL);

    // Timezone offset: +06:00 (Dhaka)
    // 2026-11-01T16:30:00+06:00 is exactly equal to 2026-11-01T10:30:00Z
    auto r4 = Timestamp::parseIso8601("2026-11-01T16:30:00+06:00");
    ASSERT_TRUE(r4.isOk());
    ASSERT_EQ(r4.value(), r2.value());

    // Leap year date: 2024-02-29T12:00:00Z
    auto rLeap = Timestamp::parseIso8601("2024-02-29T12:00:00Z");
    ASSERT_TRUE(rLeap.isOk());
}

TEST_CASE(TestTimestampParsingInvalid) {
    // Malformed strings
    ASSERT_TRUE(Timestamp::parseIso8601("").isError());
    ASSERT_TRUE(Timestamp::parseIso8601("invalid").isError());
    ASSERT_TRUE(Timestamp::parseIso8601("2026-13-01T10:00:00Z").isError()); // Month 13
    ASSERT_TRUE(Timestamp::parseIso8601("2026-02-29T10:00:00Z").isError()); // 2026 not leap year
    ASSERT_TRUE(Timestamp::parseIso8601("2026-04-31T10:00:00Z").isError()); // April has 30 days
    ASSERT_TRUE(Timestamp::parseIso8601("2026-11-01T25:00:00Z").isError()); // Hour 25
    ASSERT_TRUE(Timestamp::parseIso8601("2026-11-01T10:65:00Z").isError()); // Minute 65
    ASSERT_TRUE(Timestamp::parseIso8601("2026-11-01T10:00:60Z").isError()); // Second 60
    ASSERT_TRUE(Timestamp::parseIso8601("1969-12-31T23:59:59Z").isError()); // Prior to 1970
}

TEST_CASE(TestFakeAndSystemClock) {
    FakeClock fake(1000ULL);
    ASSERT_EQ(fake.nowMicros(), 1000ULL);
    fake.advanceMicros(500ULL);
    ASSERT_EQ(fake.nowMicros(), 1500ULL);
    fake.advanceSeconds(2);
    ASSERT_EQ(fake.nowMicros(), 2001500ULL);

    SystemClock sys;
    uint64_t t1 = sys.nowMicros();
    uint64_t t2 = sys.nowMicros();
    uint64_t t3 = sys.nowMicros();
    ASSERT_TRUE(t2 > t1);
    ASSERT_TRUE(t3 > t2);
}

TEST_CASE(TestValidation) {
    ASSERT_TRUE(Validation::validateKey("user:123").isOk());
    ASSERT_TRUE(Validation::validateKey("").isError()); // Empty key rejected
    std::string hugeKey(300, 'k');
    ASSERT_TRUE(Validation::validateKey(hugeKey).isError()); // Exceeds 256 chars

    std::string badKey = "user\x01name";
    ASSERT_TRUE(Validation::validateKey(badKey).isError()); // Control char rejected

    ASSERT_TRUE(Validation::validateValue("Hello World").isOk());
    ASSERT_TRUE(Validation::validateVersionNumber(1).isOk());
    ASSERT_TRUE(Validation::validateVersionNumber(0).isError()); // 0 is invalid

    ASSERT_TRUE(Validation::validateSnapshotName("snap_v1.0").isOk());
    ASSERT_TRUE(Validation::validateSnapshotName("").isError());
    ASSERT_TRUE(Validation::validateSnapshotName("bad snap name!").isError()); // Spaces / punctuation rejected
}

// stage1 test cases registered via TEST_CASE macros
