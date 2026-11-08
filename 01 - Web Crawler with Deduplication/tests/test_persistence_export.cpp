#include "MiniTest.h"
#include "CheckpointManager.h"
#include "Exporter.h"
#include "FakeFetcher.h"
#include <fstream>
#include <filesystem>

using namespace crawler;

TEST_CASE(TestCheckpointSaveAndLoadStateIdentity) {
    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("https://example.com/start", 200, 
        "<title>Start</title><a href='/next'>Next</a>");
    fetcher->registerResponse("https://example.com/next", 200, 
        "<title>Next</title>Leaf");

    Config cfg;
    cfg.seedUrls = {"https://example.com/start"};
    cfg.maxDepth = 2;
    cfg.maxPages = 10;

    CrawlerEngine engine1(cfg, fetcher);
    engine1.run();

    ASSERT_EQ(engine1.crawledCount(), 2u);

    std::string checkpointFile = "data/test_checkpoint.state";
    auto saveRes = CheckpointManager::save(checkpointFile, engine1);
    ASSERT_TRUE(saveRes.isOk());

    // Restore into a fresh engine
    CrawlerEngine engine2(cfg, fetcher);
    auto loadRes = CheckpointManager::load(checkpointFile, engine2);
    ASSERT_TRUE(loadRes.isOk());

    // Verify state identity
    ASSERT_EQ(engine2.storage().pageCount(), 2u);
    ASSERT_TRUE(engine2.storage().hasPage("https://example.com/start"));
    ASSERT_TRUE(engine2.storage().hasPage("https://example.com/next"));

    ASSERT_TRUE(engine2.visitedSet().contains("https://example.com/start"));
    ASSERT_TRUE(engine2.visitedSet().contains("https://example.com/next"));

    ASSERT_TRUE(engine2.bloomFilter().possiblyContains("https://example.com/start"));
    ASSERT_TRUE(engine2.bloomFilter().possiblyContains("https://example.com/next"));

    // Cleanup
    std::filesystem::remove(checkpointFile);
}

TEST_CASE(TestCheckpointCorruptionDetection) {
    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("https://example.com/doc", 200, "Content");

    Config cfg;
    cfg.seedUrls = {"https://example.com/doc"};

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    std::string checkpointFile = "data/corrupt_checkpoint.state";
    ASSERT_TRUE(CheckpointManager::save(checkpointFile, engine).isOk());

    // Corrupt a byte in the payload
    {
        std::fstream fs(checkpointFile, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(fs.is_open());
        fs.seekp(10); // Offset into payload
        char b = 0;
        fs.read(&b, 1);
        b = static_cast<char>(b ^ 0x55); // Flip bits
        fs.seekp(10);
        fs.write(&b, 1);
    }


    CrawlerEngine restoreEngine(cfg, fetcher);
    auto loadRes = CheckpointManager::load(checkpointFile, restoreEngine);
    // Must be rejected as corrupted!
    ASSERT_TRUE(loadRes.isError());
    ASSERT_EQ(loadRes.code(), StatusCode::CHECKPOINT_CORRUPTED);

    std::filesystem::remove(checkpointFile);
}

TEST_CASE(TestExportJsonAndCsv) {
    auto fetcher = std::make_shared<FakeFetcher>();
    fetcher->registerResponse("https://example.com/page", 200, 
        "<title>Hello &amp; World</title><meta name=\"description\" content=\"Test\"><a href='/out'>Out</a>");

    Config cfg;
    cfg.seedUrls = {"https://example.com/page"};

    CrawlerEngine engine(cfg, fetcher);
    engine.run();

    std::string jsonPath = "data/test_export.json";
    std::string csvPath = "data/test_export.csv";

    ASSERT_TRUE(Exporter::exportJson(jsonPath, engine).isOk());
    ASSERT_TRUE(Exporter::exportCsv(csvPath, engine).isOk());

    // Verify JSON file contains expected keys
    {
        std::ifstream jsonFile(jsonPath);
        ASSERT_TRUE(jsonFile.is_open());
        std::string jsonStr((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
        ASSERT_TRUE(jsonStr.find("\"url\": \"https://example.com/page\"") != std::string::npos);
        ASSERT_TRUE(jsonStr.find("\"title\": \"Hello & World\"") != std::string::npos);
    }

    // Verify CSV file contains header and formula escaping check
    {
        std::ifstream csvFile(csvPath);
        ASSERT_TRUE(csvFile.is_open());
        std::string csvStr((std::istreambuf_iterator<char>(csvFile)), std::istreambuf_iterator<char>());
        ASSERT_TRUE(csvStr.find("url,statusCode,depth") != std::string::npos);
    }

    // Test formula injection defense in cell escaping
    ASSERT_EQ(Exporter::escapeCsvCell("=1+2"), "'=1+2");
    ASSERT_EQ(Exporter::escapeCsvCell("+cmd"), "'+cmd");
    ASSERT_EQ(Exporter::escapeCsvCell("-eval"), "'-eval");
    ASSERT_EQ(Exporter::escapeCsvCell("@sum"), "'@sum");
    ASSERT_EQ(Exporter::escapeCsvCell("normal,cell"), "\"normal,cell\"");

    std::error_code ec;
    std::filesystem::remove(jsonPath, ec);
    std::filesystem::remove(csvPath, ec);
}
