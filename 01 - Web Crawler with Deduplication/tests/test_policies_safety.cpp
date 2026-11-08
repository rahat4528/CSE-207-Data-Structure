#include "MiniTest.h"
#include "RobotsParser.h"
#include "DomainPolicy.h"
#include "SsrfGuard.h"
#include "ContentHasher.h"
#include "RateLimiter.h"
#include "TrapDetector.h"

using namespace crawler;

TEST_CASE(TestRobotsParserRulesAndPrecedence) {
    std::string robotsTxt = R"(
        User-agent: Googlebot
        Disallow: /private/

        User-agent: *
        Disallow: /admin/
        Disallow: /temp/
        Allow: /temp/public/
        Allow: /exact/end$
        Crawl-delay: 5
    )";

    RobotsPolicy policy = RobotsPolicy::parse(robotsTxt, "SearchBot/1.0");

    // Standard Disallow
    ASSERT_FALSE(policy.isAllowed("/admin/dashboard"));
    ASSERT_FALSE(policy.isAllowed("/temp/file.txt"));

    // Longest-match: /temp/public/ (len 13) is Allow, beats /temp/ (len 6) Disallow
    ASSERT_TRUE(policy.isAllowed("/temp/public/index.html"));

    // End-anchor $: /exact/end matches, but /exact/end/other does NOT
    ASSERT_TRUE(policy.isAllowed("/exact/end"));
    ASSERT_TRUE(policy.isAllowed("/unrestricted/path"));

    // Crawl-delay parsed
    ASSERT_TRUE(policy.crawlDelaySec().has_value());
    ASSERT_EQ(policy.crawlDelaySec().value(), 5u);
}

TEST_CASE(TestDomainPolicyModesAndFilters) {
    // SAME_HOST
    DomainPolicy hostPolicy(DomainMode::SAME_HOST);
    hostPolicy.addAllowedHost("example.com");

    ASSERT_TRUE(hostPolicy.isAllowed("https://example.com/page"));
    ASSERT_FALSE(hostPolicy.isAllowed("https://sub.example.com/page")); // Subdomain rejected in SAME_HOST
    ASSERT_FALSE(hostPolicy.isAllowed("https://other.com/page"));

    // SAME_DOMAIN
    DomainPolicy domPolicy(DomainMode::SAME_DOMAIN);
    domPolicy.addAllowedDomain("example.com");

    ASSERT_TRUE(domPolicy.isAllowed("https://example.com/page"));
    ASSERT_TRUE(domPolicy.isAllowed("https://api.example.com/v1")); // Subdomain accepted
    ASSERT_TRUE(domPolicy.isAllowed("https://docs.sub.example.com/"));
    ASSERT_FALSE(domPolicy.isAllowed("https://badexample.com/")); // Not a subdomain

    // Path exclude patterns
    domPolicy.addExcludePattern("/private/*");
    ASSERT_FALSE(domPolicy.isAllowed("https://example.com/private/secret"));
    ASSERT_TRUE(domPolicy.isAllowed("https://example.com/public/data"));
}

TEST_CASE(TestSsrfGuardBlockedRanges) {
    // Loopback
    ASSERT_TRUE(SsrfGuard::isBlockedIp("127.0.0.1"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("127.255.255.254"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("::1"));

    // Private IPv4 ranges
    ASSERT_TRUE(SsrfGuard::isBlockedIp("10.0.0.1"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("172.16.0.1"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("172.31.255.255"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("192.168.1.1"));

    // Cloud metadata (AWS/GCP/Azure)
    ASSERT_TRUE(SsrfGuard::isBlockedIp("169.254.169.254"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("169.254.1.1"));

    // Carrier-grade NAT
    ASSERT_TRUE(SsrfGuard::isBlockedIp("100.64.0.1"));

    // IPv4-mapped IPv6
    ASSERT_TRUE(SsrfGuard::isBlockedIp("::ffff:127.0.0.1"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("::ffff:169.254.169.254"));
    ASSERT_TRUE(SsrfGuard::isBlockedIp("::ffff:10.1.2.3"));

    // Public Internet IPs allowed
    ASSERT_FALSE(SsrfGuard::isBlockedIp("8.8.8.8"));
    ASSERT_FALSE(SsrfGuard::isBlockedIp("1.1.1.1"));
    ASSERT_FALSE(SsrfGuard::isBlockedIp("93.184.216.34"));

    // Test override allowed when explicitly enabled for unit tests
    ASSERT_FALSE(SsrfGuard::isBlockedIp("127.0.0.1", true));
}

TEST_CASE(TestContentHasherAndSimHashNearDuplicates) {
    std::string text1 = "Modern search engines use web crawlers to discover documents across the internet. "
                        "Crawlers follow hyperlinks level by level, maintaining a queue of discovered URLs. "
                        "A Bloom filter provides fast approximate duplicate checks to avoid revisiting pages. "
                        "The exact visited set guarantees that no valid webpage is erroneously skipped during indexing.";

    std::string text2 = "Modern   search engines use web crawlers to discover documents across the internet. "
                        "Crawlers follow hyperlinks level by level, maintaining a queue of discovered URLs.   "
                        "A Bloom filter provides fast approximate duplicate checks to avoid revisiting pages. "
                        "The exact visited set guarantees that no valid webpage is erroneously skipped during indexing.";

    std::string text3 = "Modern search engines use web crawlers to discover documents across the internet. "
                        "Crawlers follow hyperlinks level by level, maintaining a queue of discovered URLs. "
                        "A Bloom filter provides rapid approximate duplicate checks to avoid revisiting pages. "
                        "The exact visited set guarantees that no valid webpage is erroneously skipped during indexing.";

    std::string text4 = "Baking chocolate chip cookies requires flour, butter, sugar, and semi-sweet chocolate chips. "
                        "Preheat the oven to 375 degrees Fahrenheit and mix dry ingredients in a large bowl.";

    auto fp1 = ContentHasher::compute(text1);
    auto fp2 = ContentHasher::compute(text2);
    auto fp3 = ContentHasher::compute(text3);
    auto fp4 = ContentHasher::compute(text4);

    // Exact hash matches despite whitespace
    ASSERT_EQ(fp1.exactHash64, fp2.exactHash64);

    // Text 1 and Text 3 are near-duplicates
    uint32_t dist13 = ContentHasher::hammingDistance(fp1.simHash64, fp3.simHash64);
    ASSERT_TRUE(dist13 <= 3);
    ASSERT_TRUE(ContentHasher::isNearDuplicate(fp1.simHash64, fp3.simHash64));

    // Text 1 and Text 4 are NOT near-duplicates
    uint32_t dist14 = ContentHasher::hammingDistance(fp1.simHash64, fp4.simHash64);
    ASSERT_TRUE(dist14 > 10);
    ASSERT_FALSE(ContentHasher::isNearDuplicate(fp1.simHash64, fp4.simHash64));
}


TEST_CASE(TestRateLimiterAndBackoff) {
    RateLimiter limiter(100); // 100ms

    // Initial wait is 0
    ASSERT_EQ(limiter.getWaitTimeMs("example.com"), 0u);

    limiter.recordRequest("example.com");
    // Immediate subsequent wait should be > 0 and <= 100
    uint32_t wait = limiter.getWaitTimeMs("example.com");
    ASSERT_TRUE(wait > 0 && wait <= 100);

    // Retry-After overrides exponential calculation
    uint32_t retryWait = RateLimiter::calculateBackoffMs(1, 1000, 30);
    ASSERT_EQ(retryWait, 30000u); // 30s * 1000ms

    // Exponential backoff increases with attempts
    uint32_t b0 = RateLimiter::calculateBackoffMs(0, 1000);
    uint32_t b2 = RateLimiter::calculateBackoffMs(2, 1000);
    ASSERT_TRUE(b2 > b0);
}

TEST_CASE(TestTrapDetectorRepeatingSegments) {
    // Repeating segments loop
    ASSERT_TRUE(TrapDetector::hasRepeatingSegments("/calendar/2024/2024/2024/day", 3));
    ASSERT_TRUE(TrapDetector::hasRepeatingSegments("/category/shoes/category/shoes/category/shoes", 3));
    ASSERT_FALSE(TrapDetector::hasRepeatingSegments("/about/us/contact", 3));

    // Excessive segment depth
    ASSERT_TRUE(TrapDetector::hasTooManySegments("/1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16", 15));
    ASSERT_FALSE(TrapDetector::hasTooManySegments("/docs/api/v1/users", 15));
}
