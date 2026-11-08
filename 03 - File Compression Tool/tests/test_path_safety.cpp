#include "TestRunner.h"
#include "PathSanitizer.h"
#include "BatchProcessor.h"
#include <filesystem>
#include <fstream>

using namespace HuffmanEngine;

TEST_CASE(test_path_sanitizer_rules) {
    // 1. Basic filename
    ASSERT_EQ(PathSanitizer::sanitizeFilename("hello.txt"), "hello.txt");

    // 2. Directory traversal attempt
    ASSERT_EQ(PathSanitizer::sanitizeFilename("../../secret/config.cfg"), "config.cfg");
    ASSERT_EQ(PathSanitizer::sanitizeFilename("..\\..\\nested_folder\\data.dat"), "data.dat");

    // 3. Windows reserved names
    ASSERT_EQ(PathSanitizer::sanitizeFilename("CON"), "safe_CON");
    ASSERT_EQ(PathSanitizer::sanitizeFilename("nul.txt"), "safe_nul.txt");
    ASSERT_EQ(PathSanitizer::sanitizeFilename("aux.dat"), "safe_aux.dat");

    // 4. Safe output path resolution
    std::filesystem::path base = "test_out/safe_dir";
    std::filesystem::create_directories(base);

    auto safeRes = PathSanitizer::resolveSafeOutputPath(base, "normal.txt");
    ASSERT_TRUE(safeRes.isOk);
    ASSERT_EQ(safeRes.value.filename().string(), "normal.txt");

    // Traversal inside base must be sanitized to stay inside base
    auto travRes = PathSanitizer::resolveSafeOutputPath(base, "../../../bad.txt");
    ASSERT_TRUE(travRes.isOk);
    ASSERT_EQ(travRes.value.filename().string(), "bad.txt");
    ASSERT_EQ(travRes.value.parent_path(), base);
}

TEST_CASE(test_overwrite_protection_and_force) {
    std::filesystem::path testDir = "test_out";
    std::filesystem::create_directories(testDir);
    std::filesystem::path src = testDir / "src_test.txt";
    std::filesystem::path out = testDir / "out_test.txt";

    {
        std::ofstream(src) << "Source Content";
        std::ofstream(out) << "Existing Target Content";
    }

    // 1. Same input and output must fail
    auto sameRes = PathSanitizer::validateOutputFile(src, src, true);
    ASSERT_FALSE(sameRes.isOk);
    ASSERT_EQ(sameRes.error, ErrorCode::ERR_INVALID_PARAMETER);

    // 2. Existing output without force must fail
    auto existRes = PathSanitizer::validateOutputFile(src, out, false);
    ASSERT_FALSE(existRes.isOk);
    ASSERT_EQ(existRes.error, ErrorCode::ERR_FILE_EXISTS);

    // 3. Existing output WITH force must succeed
    auto forceRes = PathSanitizer::validateOutputFile(src, out, true);
    ASSERT_TRUE(forceRes.isOk);

    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST_CASE(test_batch_directory_compression_and_decompression) {
    std::filesystem::path batchSrc = "test_out/batch_src";
    std::filesystem::path batchArc = "test_out/batch_arc";
    std::filesystem::path batchOut = "test_out/batch_out";

    std::filesystem::create_directories(batchSrc);
    std::filesystem::create_directories(batchArc);
    std::filesystem::create_directories(batchOut);

    // Create 3 sample files
    {
        std::ofstream(batchSrc / "doc1.txt") << "First document in batch test.";
        std::ofstream(batchSrc / "doc2.txt") << "Second document with slightly different text contents.";
        std::ofstream(batchSrc / "doc3.txt") << "Third document, repetitively: AAAAABBBBBCCCCCDDDDD.";
    }

    // Batch compress
    auto compSummary = BatchProcessor::compressDirectory(batchSrc, batchArc, /*recursive=*/false, /*force=*/true);
    ASSERT_EQ(compSummary.totalFiles, 3);
    ASSERT_EQ(compSummary.successfulFiles, 3);
    ASSERT_EQ(compSummary.failedFiles, 0);

    // Batch decompress
    auto decompSummary = BatchProcessor::decompressDirectory(batchArc, batchOut, /*recursive=*/false, /*force=*/true);
    ASSERT_EQ(decompSummary.totalFiles, 3);
    ASSERT_EQ(decompSummary.successfulFiles, 3);
    ASSERT_EQ(decompSummary.failedFiles, 0);

    // Verify contents match
    std::string s1, s2;
    std::ifstream(batchSrc / "doc1.txt") >> s1;
    std::ifstream(batchOut / "doc1.txt") >> s2;
    ASSERT_EQ(s1, s2);

    // Clean up
    std::filesystem::remove_all(batchSrc);
    std::filesystem::remove_all(batchArc);
    std::filesystem::remove_all(batchOut);
}
