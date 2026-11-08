#include "CrawlerEngine.h"
#include "HtmlParser.h"
#include "UrlNormalizer.h"
#include "ContentHasher.h"
#include "Logger.h"

namespace crawler {

CrawlerEngine::CrawlerEngine(Config config, std::shared_ptr<IFetcher> fetcher)
    : m_config(std::move(config)),
      m_fetcher(std::move(fetcher)),
      m_bloom(m_config.bloomCapacity, m_config.bloomTargetFpr),
      m_scheduler(m_config.maxDepth, m_config.maxPages) {}

void CrawlerEngine::init() {
    m_startTime = std::chrono::steady_clock::now();
    m_hasStarted = true;
    m_hasFinished = false;

    for (const auto& seed : m_config.seedUrls) {
        auto norm = UrlNormalizer::normalize(seed);
        if (norm.isOk()) {
            std::string canonicalSeed = norm.value();
            m_visited.insert(canonicalSeed);
            m_bloom.insert(canonicalSeed);
            m_scheduler.seed(canonicalSeed);
            m_graph.addNode(canonicalSeed);
        }
    }
}

bool CrawlerEngine::step() {
    if (!m_hasStarted) {
        m_startTime = std::chrono::steady_clock::now();
        m_hasStarted = true;
    }

    auto taskOpt = m_scheduler.nextTask();
    if (!taskOpt.has_value()) {
        m_hasFinished = true;
        m_endTime = std::chrono::steady_clock::now();
        return false; // Crawl finished
    }

    CrawlTask task = taskOpt.value();

    FetchRequest req;
    req.url = task.url;
    req.maxRedirects = m_config.maxRedirects;
    req.maxSizeBytes = m_config.maxResponseSizeBytes;
    req.connectTimeoutMs = m_config.connectTimeoutMs;
    req.totalTimeoutMs = m_config.totalTimeoutMs;
    req.headers["User-Agent"] = m_config.userAgent;

    auto res = m_fetcher->fetch(req);
    m_scheduler.markCrawled();

    if (res.isError()) {
        LOG_WARN("Fetch failed for " << task.url << ": " << res.errorMessage());
        CrawledPage failedPage;
        failedPage.url = task.url;
        failedPage.statusCode = -1;
        failedPage.depth = task.depth;
        failedPage.cleanText = res.errorMessage();
        m_storage.storePage(std::move(failedPage));
        m_graph.addNode(task.url);
        return true;
    }

    FetchResponse resp = std::move(res.value());
    std::string pageUrl = resp.finalUrl.empty() ? task.url : resp.finalUrl;
    m_graph.addNode(pageUrl);
    m_totalBytesFetched += resp.body.size();

    if (resp.isSuccess()) {
        ParsedDocument doc = HtmlParser::parse(pageUrl, resp.body);

        CrawledPage pageRecord;
        pageRecord.url = pageUrl;
        pageRecord.statusCode = resp.statusCode;
        pageRecord.depth = task.depth;
        pageRecord.title = doc.title;
        pageRecord.metaDescription = doc.metaDescription;
        pageRecord.cleanText = doc.cleanText;
        pageRecord.contentLength = resp.body.size();
        pageRecord.contentHash = ContentHasher::computeExactHash(doc.cleanText);
        pageRecord.simHash = ContentHasher::computeSimHash(doc.cleanText);

        for (const auto& link : doc.links) {
            std::string target = link.targetUrl;
            m_graph.addEdge(pageUrl, target);
            pageRecord.outLinks.push_back(target);

            // Respect rel="nofollow" or meta nofollow
            if (link.nofollow) {
                continue;
            }

            // Deduplication logic
            m_candidateUrlsSeen++;
            bool bloomSaysMaybe = m_bloom.possiblyContains(target);
            if (!bloomSaysMaybe) {
                // Definitely new URL
                m_bloomNegatives++;
                m_bloom.insert(target);
                m_visited.insert(target);
                m_scheduler.enqueueDiscovered(target, task.depth);
            } else {
                // Possibly seen: must consult Exact Visited Set
                m_bloomPositives++;
                if (!m_visited.contains(target)) {
                    // Bloom false positive!
                    m_bloomFalsePositives++;
                    m_visited.insert(target);
                    m_scheduler.enqueueDiscovered(target, task.depth);
                } else {
                    m_exactDuplicates++;
                }
            }
        }

        m_storage.storePage(std::move(pageRecord));
    } else {
        CrawledPage errPage;
        errPage.url = pageUrl;
        errPage.statusCode = resp.statusCode;
        errPage.depth = task.depth;
        m_storage.storePage(std::move(errPage));
    }

    return true;
}

void CrawlerEngine::run() {
    init();
    while (step()) {
        // Run until scheduler finishes
    }
    m_hasFinished = true;
    m_endTime = std::chrono::steady_clock::now();
}

double CrawlerEngine::elapsedSeconds() const noexcept {
    if (!m_hasStarted) return 0.0;
    auto endPoint = m_hasFinished ? m_endTime : std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(endPoint - m_startTime);
    return static_cast<double>(dur.count()) / 1000000.0;
}

} // namespace crawler

