#include "TestRunner.h"
#include "Compressor.h"
#include "Decompressor.h"
#include "FileIO.h"
#include <filesystem>
#include <random>

using namespace HuffmanEngine;

TEST_CASE(test_streaming_large_file_disk_roundtrip) {
    std::filesystem::path testDir = "test_out";
    std::filesystem::create_directories(testDir);

    std::filesystem::path rawPath = testDir / "large_stream_test.dat";
    std::filesystem::path huffPath = testDir / "large_stream_test.dat.huff";
    std::filesystem::path restoredPath = testDir / "large_stream_restored.dat";

    // Clean up any earlier leftovers
    std::filesystem::remove(rawPath);
    std::filesystem::remove(huffPath);
    std::filesystem::remove(restoredPath);

    // Generate 2 MB of realistic repeating text data
    constexpr size_t TARGET_SIZE = 2 * 1024 * 1024; // 2 MB
    const std::string phrase = "Huffman coding is an optimal prefix code commonly used for lossless data compression. ";
    {
        std::ofstream out(rawPath, std::ios::binary);
        size_t written = 0;
        while (written < TARGET_SIZE) {
            out.write(phrase.data(), phrase.size());
            written += phrase.size();
        }
    }

    ASSERT_TRUE(std::filesystem::exists(rawPath));
    filesize_t rawSize = std::filesystem::file_size(rawPath);

    // Compress using buffered streaming File I/O
    {
        FileInputStream inStream(rawPath);
        ASSERT_TRUE(inStream.isOpen());
        FileOutputStream outStream(huffPath, /*atomicTemp=*/true);
        ASSERT_TRUE(outStream.isOpen());

        auto compRes = Compressor::compress(inStream, outStream, rawPath.filename().string());
        ASSERT_TRUE(compRes.isOk);
        auto commitRes = outStream.commit();
        ASSERT_TRUE(commitRes.isOk);

        // Huffman compression on repetitive text should achieve significant savings
        ASSERT_LT(compRes.value.compressedSize, rawSize);
        ASSERT_GT(compRes.value.spaceSavings, 30.0);
    }

    ASSERT_TRUE(std::filesystem::exists(huffPath));

    // Decompress using buffered streaming File I/O
    {
        FileInputStream arcIn(huffPath);
        ASSERT_TRUE(arcIn.isOpen());
        FileOutputStream outRestored(restoredPath, /*atomicTemp=*/true);
        ASSERT_TRUE(outRestored.isOpen());

        auto decompRes = Decompressor::decompress(arcIn, outRestored);
        ASSERT_TRUE(decompRes.isOk);
        auto commitRes = outRestored.commit();
        ASSERT_TRUE(commitRes.isOk);

        ASSERT_EQ(decompRes.value.decompressedSize, rawSize);
    }

    ASSERT_TRUE(std::filesystem::exists(restoredPath));
    ASSERT_EQ(std::filesystem::file_size(restoredPath), rawSize);

    // Stream-based byte-by-byte comparison
    {
        FileInputStream f1(rawPath);
        FileInputStream f2(restoredPath);
        std::vector<uint8_t> buf1(DEFAULT_BUFFER_SIZE);
        std::vector<uint8_t> buf2(DEFAULT_BUFFER_SIZE);

        filesize_t totalChecked = 0;
        while (!f1.isEof() && !f2.isEof()) {
            size_t n1 = f1.read(buf1.data(), DEFAULT_BUFFER_SIZE);
            size_t n2 = f2.read(buf2.data(), DEFAULT_BUFFER_SIZE);
            ASSERT_EQ(n1, n2);
            if (n1 == 0) break;
            ASSERT_TRUE(std::memcmp(buf1.data(), buf2.data(), n1) == 0);
            totalChecked += n1;
        }
        ASSERT_EQ(totalChecked, rawSize);
    }

    // Cleanup generated large test files
    std::filesystem::remove(rawPath);
    std::filesystem::remove(huffPath);
    std::filesystem::remove(restoredPath);
}
