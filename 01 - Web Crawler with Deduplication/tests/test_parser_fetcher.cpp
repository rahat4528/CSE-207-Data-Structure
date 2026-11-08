#include "MiniTest.h"
#include "HtmlParser.h"
#include "FakeFetcher.h"

using namespace crawler;

TEST_CASE(TestHtmlParserMetadataAndLinks) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>My Cool Webpage &amp; Search</title>
            <meta name="description" content="Discover search algorithms &amp; web crawling">
            <meta name="robots" content="index, follow">
            <link rel="canonical" href="https://example.com/canonical-page">
            <base href="https://example.com/docs/">
        </head>
        <body>
            <h1>Welcome to Crawler</h1>
            <p>Here are some links:</p>
            <a href="guide.html">User Guide</a>
            <a href="../about.html" rel="nofollow">About Us</a>
            <a href="https://external.org/test" rel="external">External</a>
            <script>var x = "<a href='should_be_ignored'></a>";</script>
            <style>body { font-size: 14px; }</style>
        </body>
        </html>
    )";

    ParsedDocument doc = HtmlParser::parse("https://example.com/original.html", html);

    ASSERT_EQ(doc.title, "My Cool Webpage & Search");
    ASSERT_EQ(doc.metaDescription, "Discover search algorithms & web crawling");
    ASSERT_FALSE(doc.metaNoindex);
    ASSERT_FALSE(doc.metaNofollow);
    ASSERT_EQ(doc.canonicalUrl, "https://example.com/canonical-page");
    ASSERT_EQ(doc.baseHref, "https://example.com/docs/");

    // Links should be resolved against baseHref ("https://example.com/docs/")
    ASSERT_EQ(doc.links.size(), 3u);

    // Link 1: "guide.html" resolved to "https://example.com/docs/guide.html"
    ASSERT_EQ(doc.links[0].targetUrl, "https://example.com/docs/guide.html");
    ASSERT_FALSE(doc.links[0].nofollow);

    // Link 2: "../about.html" resolved to "https://example.com/about.html" with nofollow=true
    ASSERT_EQ(doc.links[1].targetUrl, "https://example.com/about.html");
    ASSERT_TRUE(doc.links[1].nofollow);

    // Link 3: absolute URL
    ASSERT_EQ(doc.links[2].targetUrl, "https://external.org/test");
    ASSERT_FALSE(doc.links[2].nofollow);

    // Clean text contains body text but excludes <script> and <style>
    ASSERT_TRUE(doc.cleanText.find("Welcome to Crawler") != std::string::npos);
    ASSERT_TRUE(doc.cleanText.find("should_be_ignored") == std::string::npos);
    ASSERT_TRUE(doc.cleanText.find("font-size") == std::string::npos);
}

TEST_CASE(TestHtmlParserMalformedTolerance) {
    // Malformed HTML: unquoted attributes, unclosed tags, broken entities
    std::string badHtml = R"(
        <html>
        <head><title>Unclosed Title
        <body>
            <a href=page1.html>Link 1
            <a href="page2.html>Broken Quote
            <p>Some text &amp; &lt;not_a_tag&gt;
            <meta name=robots content=noindex,nofollow>
        <!-- unclosed comment at EOF
    )";

    ParsedDocument doc = HtmlParser::parse("https://example.com/base/", badHtml);
    ASSERT_TRUE(doc.metaNoindex);
    ASSERT_TRUE(doc.metaNofollow);

    // Must not crash and should extract Link 1
    ASSERT_TRUE(!doc.links.empty());
    ASSERT_EQ(doc.links[0].targetUrl, "https://example.com/base/page1.html");
    ASSERT_TRUE(doc.links[0].nofollow); // inherited from doc.metaNofollow
}

TEST_CASE(TestFakeFetcherBasicResponse) {
    FakeFetcher fetcher;
    fetcher.registerResponse("https://example.com/index.html", 200, "<h1>Hello World</h1>");

    FetchRequest req;
    req.url = "https://example.com/index.html";

    auto res = fetcher.fetch(req);
    ASSERT_TRUE(res.isOk());

    FetchResponse resp = res.value();
    ASSERT_EQ(resp.statusCode, 200);
    ASSERT_TRUE(resp.isSuccess());
    ASSERT_EQ(resp.body, "<h1>Hello World</h1>");
    ASSERT_EQ(resp.finalUrl, "https://example.com/index.html");
}

TEST_CASE(TestFakeFetcherRedirectFollowingAndLoops) {
    FakeFetcher fetcher;
    // 3-hop redirect chain: /a -> /b -> /c (200)
    fetcher.registerRedirect("https://example.com/a", "/b", 301);
    fetcher.registerRedirect("https://example.com/b", "/c", 302);
    fetcher.registerResponse("https://example.com/c", 200, "Target page");

    FetchRequest req;
    req.url = "https://example.com/a";
    req.maxRedirects = 5;

    auto res = fetcher.fetch(req);
    ASSERT_TRUE(res.isOk());
    ASSERT_EQ(res.value().statusCode, 200);
    ASSERT_EQ(res.value().finalUrl, "https://example.com/c");
    ASSERT_EQ(res.value().redirectChain.size(), 2u);

    // Circular redirect loop: /loop1 <-> /loop2
    fetcher.registerRedirectLoop("https://example.com/loop1", "https://example.com/loop2");
    FetchRequest loopReq;
    loopReq.url = "https://example.com/loop1";
    loopReq.maxRedirects = 10;

    auto loopRes = fetcher.fetch(loopReq);
    ASSERT_TRUE(loopRes.isError());
    ASSERT_EQ(loopRes.code(), StatusCode::MAX_REDIRECTS_EXCEEDED);
}

TEST_CASE(TestFakeFetcherTimeoutsAndSizeLimits) {
    FakeFetcher fetcher;
    fetcher.registerTimeout("https://example.com/slow");
    fetcher.registerOversized("https://example.com/huge", 1024 * 1024); // 1 MiB

    // Timeout
    FetchRequest reqTimeout;
    reqTimeout.url = "https://example.com/slow";
    auto rTimeout = fetcher.fetch(reqTimeout);
    ASSERT_TRUE(rTimeout.isError());
    ASSERT_EQ(rTimeout.code(), StatusCode::READ_TIMEOUT);

    // Size cap enforcement
    FetchRequest reqSize;
    reqSize.url = "https://example.com/huge";
    reqSize.maxSizeBytes = 1024; // 1 KiB limit
    auto rSize = fetcher.fetch(reqSize);
    ASSERT_TRUE(rSize.isOk());
    ASSERT_EQ(rSize.value().body.size(), 1024u); // Cut off at 1 KiB
}

TEST_CASE(TestFakeFetcherRateLimit429) {
    FakeFetcher fetcher;
    fetcher.registerRateLimit("https://example.com/api", 60);

    FetchRequest req;
    req.url = "https://example.com/api";
    auto res = fetcher.fetch(req);
    ASSERT_TRUE(res.isOk());
    ASSERT_EQ(res.value().statusCode, 429);
    ASSERT_TRUE(res.value().isTooManyRequests());
    ASSERT_TRUE(res.value().retryAfterSec.has_value());
    ASSERT_EQ(res.value().retryAfterSec.value(), 60u);
}

