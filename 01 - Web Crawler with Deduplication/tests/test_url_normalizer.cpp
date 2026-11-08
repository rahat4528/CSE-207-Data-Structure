#include "MiniTest.h"
#include "Url.h"
#include "UrlNormalizer.h"

using namespace crawler;

TEST_CASE(TestUrlParsingValidAndInvalid) {
    // Valid URLs
    auto u1 = Url::parse("https://example.com");
    ASSERT_TRUE(u1.isOk());
    ASSERT_EQ(u1.value().scheme(), "https");
    ASSERT_EQ(u1.value().host(), "example.com");
    ASSERT_EQ(u1.value().port(), 443);
    ASSERT_EQ(u1.value().path(), "/");

    auto u2 = Url::parse("http://example.com:8080/path/to/page?query=1#frag");
    ASSERT_TRUE(u2.isOk());
    ASSERT_EQ(u2.value().port(), 8080);
    ASSERT_EQ(u2.value().path(), "/path/to/page");
    ASSERT_EQ(u2.value().query(), "query=1");
    ASSERT_EQ(u2.value().fragment(), "frag");

    // Invalid URLs
    ASSERT_TRUE(Url::parse("").isError());
    ASSERT_TRUE(Url::parse("not-a-url").isError());
    ASSERT_TRUE(Url::parse("http://").isError());
    ASSERT_TRUE(Url::parse("http://bad host.com").isError()); // space in host
    ASSERT_TRUE(Url::parse("http://example.com:9999999").isError()); // port out of range

    // Overlong URL
    std::string hugeUrl = "https://example.com/" + std::string(3000, 'a');
    ASSERT_TRUE(Url::parse(hugeUrl).isError());
}

TEST_CASE(TestUnsupportedSchemesRejected) {
    ASSERT_TRUE(Url::parse("javascript:alert(1)").isError());
    ASSERT_EQ(Url::parse("javascript:alert(1)").code(), StatusCode::UNSUPPORTED_SCHEME);

    ASSERT_TRUE(Url::parse("mailto:user@example.com").isError());
    ASSERT_EQ(Url::parse("mailto:user@example.com").code(), StatusCode::UNSUPPORTED_SCHEME);

    ASSERT_TRUE(Url::parse("tel:+1234567890").isError());
    ASSERT_EQ(Url::parse("tel:+1234567890").code(), StatusCode::UNSUPPORTED_SCHEME);

    ASSERT_TRUE(Url::parse("data:text/html,Hello").isError());
    ASSERT_EQ(Url::parse("data:text/html,Hello").code(), StatusCode::UNSUPPORTED_SCHEME);

    ASSERT_TRUE(Url::parse("file:///etc/passwd").isError());
    ASSERT_EQ(Url::parse("file:///etc/passwd").code(), StatusCode::UNSUPPORTED_SCHEME);

    ASSERT_TRUE(Url::parse("ftp://example.com").isError());
    ASSERT_EQ(Url::parse("ftp://example.com").code(), StatusCode::UNSUPPORTED_SCHEME);
}

TEST_CASE(TestCaseAndDefaultPortNormalization) {
    // Upper case scheme & host -> lower case
    auto r1 = UrlNormalizer::normalize("HTTP://EXAMPLE.COM/Page");
    ASSERT_TRUE(r1.isOk());
    ASSERT_EQ(r1.value(), "http://example.com/Page");

    // Port 80 for http stripped
    auto r2 = UrlNormalizer::normalize("http://example.com:80/index.html");
    ASSERT_TRUE(r2.isOk());
    ASSERT_EQ(r2.value(), "http://example.com/index.html");

    // Port 443 for https stripped
    auto r3 = UrlNormalizer::normalize("https://example.com:443/secure");
    ASSERT_TRUE(r3.isOk());
    ASSERT_EQ(r3.value(), "https://example.com/secure");

    // Non-default port preserved
    auto r4 = UrlNormalizer::normalize("http://example.com:8080/test");
    ASSERT_TRUE(r4.isOk());
    ASSERT_EQ(r4.value(), "http://example.com:8080/test");

    // Fragment stripped
    auto r5 = UrlNormalizer::normalize("https://example.com/docs#installation");
    ASSERT_TRUE(r5.isOk());
    ASSERT_EQ(r5.value(), "https://example.com/docs");
}

TEST_CASE(TestDotSegmentResolution) {
    ASSERT_EQ(UrlNormalizer::removeDotSegments("/a/b/c/./../../g"), "/a/g");
    ASSERT_EQ(UrlNormalizer::removeDotSegments("/b/c/../d"), "/b/d");
    ASSERT_EQ(UrlNormalizer::removeDotSegments("/../../a"), "/a");
    ASSERT_EQ(UrlNormalizer::removeDotSegments("/."), "/");
    ASSERT_EQ(UrlNormalizer::removeDotSegments("/a/./b/../c"), "/a/c");
}

TEST_CASE(TestPercentEncodingNormalization) {
    // Unreserved characters: ALPHA, DIGIT, '-', '.', '_', '~'
    // %41 -> 'A', %42 -> 'B', %31 -> '1', %7E -> '~', %5F -> '_'
    std::string raw = "/test/%41%42%43/%7Euser/%5Fdata";
    std::string norm = UrlNormalizer::normalizePercentEncoding(raw);
    ASSERT_EQ(norm, "/test/ABC/~user/_data");

    // Reserved characters stay encoded with uppercase hex
    std::string resEnc = "/path/%2f%20%3a";
    std::string normRes = UrlNormalizer::normalizePercentEncoding(resEnc);
    ASSERT_EQ(normRes, "/path/%2F%20%3A");
}

TEST_CASE(TestQueryStringSortingAndTrackingStripping) {
    std::string q = "utm_source=google&b=2&a=1&utm_medium=cpc&c=3&ref=producthunt";
    std::string normQ = UrlNormalizer::normalizeQueryString(q);
    ASSERT_EQ(normQ, "a=1&b=2&c=3");

    // When only tracking parameters exist, query becomes empty
    std::string onlyTracking = "utm_source=newsletter&utm_campaign=winter";
    ASSERT_EQ(UrlNormalizer::normalizeQueryString(onlyTracking), "");
}

TEST_CASE(TestRelativeUrlResolution) {
    std::string base = "https://example.com/sub/dir/page.html?x=1";

    // Root-relative
    auto rRoot = UrlNormalizer::resolve(base, "/about");
    ASSERT_TRUE(rRoot.isOk());
    ASSERT_EQ(rRoot.value(), "https://example.com/about");

    // Directory-relative
    auto rDir = UrlNormalizer::resolve(base, "other.html");
    ASSERT_TRUE(rDir.isOk());
    ASSERT_EQ(rDir.value(), "https://example.com/sub/dir/other.html");

    // Parent directory-relative
    auto rParent = UrlNormalizer::resolve(base, "../index.html");
    ASSERT_TRUE(rParent.isOk());
    ASSERT_EQ(rParent.value(), "https://example.com/sub/index.html");

    // Protocol-relative
    auto rProto = UrlNormalizer::resolve(base, "//cdn.example.com/lib.js");
    ASSERT_TRUE(rProto.isOk());
    ASSERT_EQ(rProto.value(), "https://cdn.example.com/lib.js");

    // Query-relative
    auto rQuery = UrlNormalizer::resolve(base, "?y=2");
    ASSERT_TRUE(rQuery.isOk());
    ASSERT_EQ(rQuery.value(), "https://example.com/sub/dir/page.html?y=2");

    // Base href resolution
    auto rBaseHref = UrlNormalizer::resolveWithBaseHref(base, "https://example.com/assets/", "style.css");
    ASSERT_TRUE(rBaseHref.isOk());
    ASSERT_EQ(rBaseHref.value(), "https://example.com/assets/style.css");
}

TEST_CASE(TestNormalizationIdempotence) {
    std::vector<std::string> testUrls = {
        "http://EXAMPLE.COM:80/a/b/../c/./d?utm_source=fb&z=9&a=1#frag",
        "https://test.org:443/%7Euser/./index.html?ref=abc&b=2&a=1",
        "http://sub.domain.co.uk/path/to/resource/?b=1&a=2",
        "https://example.com/test/%41%42%43"
    };

    for (const auto& raw : testUrls) {
        auto r1 = UrlNormalizer::normalize(raw);
        ASSERT_TRUE(r1.isOk());
        auto r2 = UrlNormalizer::normalize(r1.value());
        ASSERT_TRUE(r2.isOk());
        ASSERT_EQ(r1.value(), r2.value()); // normalize(normalize(u)) == normalize(u)
    }
}

