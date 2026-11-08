#include "MiniTest.h"
#include "Common.h"
#include "CRC32.h"
#include "Logger.h"
#include "Config.h"

using namespace crawler;

TEST_CASE(TestResultMonad) {
    auto okRes = Result<int>::ok(42);
    ASSERT_TRUE(okRes.isOk());
    ASSERT_FALSE(okRes.isError());
    ASSERT_EQ(okRes.value(), 42);

    auto errRes = Result<int>::error(StatusCode::INVALID_URL, "Bad URL provided");
    ASSERT_TRUE(errRes.isError());
    ASSERT_FALSE(errRes.isOk());
    ASSERT_EQ(errRes.code(), StatusCode::INVALID_URL);
    ASSERT_EQ(errRes.errorMessage(), "Bad URL provided");

    auto voidOk = Result<void>::ok();
    ASSERT_TRUE(voidOk.isOk());

    auto voidErr = Result<void>::error(StatusCode::SSRF_BLOCKED, "Blocked IP range");
    ASSERT_TRUE(voidErr.isError());
    ASSERT_EQ(voidErr.code(), StatusCode::SSRF_BLOCKED);
}

TEST_CASE(TestCRC32KnownVectors) {
    const CRC32& crc = getCRC32Instance();

    // Standard IEEE 802.3 test vectors:
    // "123456789" -> 0xCBF43926
    uint32_t c1 = crc.calculate("123456789");
    ASSERT_EQ(c1, 0xCBF43926U);

    // "" (empty string) -> 0x00000000
    uint32_t c2 = crc.calculate("");
    ASSERT_EQ(c2, 0x00000000U);

    // Incremental update test
    uint32_t runningCrc = 0xFFFFFFFFU;
    const char part1[] = "12345";
    const char part2[] = "6789";
    runningCrc = crc.update(runningCrc, reinterpret_cast<const uint8_t*>(part1), 5);
    runningCrc = crc.update(runningCrc, reinterpret_cast<const uint8_t*>(part2), 4);
    runningCrc ^= 0xFFFFFFFFU;
    ASSERT_EQ(runningCrc, 0xCBF43926U);
}

TEST_CASE(TestConfigDefaultsAndValidation) {
    Config cfg;
    // Without seeds, validate fails
    ASSERT_TRUE(cfg.validate().isError());

    cfg.seedUrls.push_back("https://example.com");
    ASSERT_TRUE(cfg.validate().isOk());

    // Bad concurrency
    cfg.concurrency = 0;
    ASSERT_TRUE(cfg.validate().isError());
    cfg.concurrency = 4;

    // Bad bloom capacity
    cfg.bloomCapacity = 0;
    ASSERT_TRUE(cfg.validate().isError());
    cfg.bloomCapacity = 10000;

    // Bad bloom target FPR
    cfg.bloomTargetFpr = 0.0;
    ASSERT_TRUE(cfg.validate().isError());
    cfg.bloomTargetFpr = 1.0;
    ASSERT_TRUE(cfg.validate().isError());
    cfg.bloomTargetFpr = 0.01;
    ASSERT_TRUE(cfg.validate().isOk());
}

TEST_CASE(TestCommandLineParsing) {
    const char* argv[] = {
        "crawler.exe",
        "--seed", "https://crawler-test.com",
        "--depth", "5",
        "--pages", "50",
        "--concurrency", "8",
        "--delay", "500",
        "--domain-mode", "same_host",
        "--no-robots"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);

    auto r = Config::parseCommandLine(argc, const_cast<char**>(argv));
    ASSERT_TRUE(r.isOk());

    Config cfg = r.value();
    ASSERT_EQ(cfg.seedUrls.size(), 1u);
    ASSERT_EQ(cfg.seedUrls[0], "https://crawler-test.com");
    ASSERT_EQ(cfg.maxDepth, 5u);
    ASSERT_EQ(cfg.maxPages, 50u);
    ASSERT_EQ(cfg.concurrency, 8u);
    ASSERT_EQ(cfg.perHostDelayMs, 500u);
    ASSERT_TRUE(cfg.domainMode == DomainMode::SAME_HOST);
    ASSERT_FALSE(cfg.respectRobots);
}

int main() {
    return minitest::TestRegistry::instance().runAll();
}

