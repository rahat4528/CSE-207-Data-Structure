#include "TestRunner.h"
#include "Compressor.h"
#include "Decompressor.h"
#include <random>

using namespace HuffmanEngine;

// Helper to run end-to-end compression and decompression on a byte buffer
static void verifyRoundTrip(const std::vector<uint8_t>& original, const std::string& name = "sample.bin") {
    MemoryInputStream srcStream(original);
    MemoryOutputStream archiveStream;

    auto compRes = Compressor::compress(srcStream, archiveStream, name);
    ASSERT_TRUE(compRes.isOk);

    MemoryInputStream arcInStream(archiveStream.getBuffer());
    MemoryOutputStream restoredStream;

    auto decompRes = Decompressor::decompress(arcInStream, restoredStream);
    ASSERT_TRUE(decompRes.isOk);

    ASSERT_EQ(decompRes.value.decompressedSize, original.size());
    ASSERT_EQ(restoredStream.getBuffer().size(), original.size());
    ASSERT_TRUE(restoredStream.getBuffer() == original);
}

TEST_CASE(test_roundtrip_empty_file) {
    std::vector<uint8_t> emptyData;
    verifyRoundTrip(emptyData, "empty.txt");
}

TEST_CASE(test_roundtrip_single_char_files) {
    // 1 single character
    std::vector<uint8_t> singleByte = {'X'};
    verifyRoundTrip(singleByte, "single.txt");

    // 50,000 repetitions of one byte
    std::vector<uint8_t> repeated(50000, 'M');
    verifyRoundTrip(repeated, "repeated.txt");
}

TEST_CASE(test_roundtrip_two_symbols) {
    std::vector<uint8_t> twoSyms;
    for (int i = 0; i < 2000; ++i) {
        twoSyms.push_back((i % 2 == 0) ? '0' : '1');
    }
    verifyRoundTrip(twoSyms, "binary_two.txt");
}

TEST_CASE(test_roundtrip_all_256_byte_values) {
    std::vector<uint8_t> allBytes;
    for (int rep = 0; rep < 5; ++rep) {
        for (int b = 0; b < 256; ++b) {
            allBytes.push_back(static_cast<uint8_t>(b));
        }
    }
    verifyRoundTrip(allBytes, "all_256_bytes.bin");
}

TEST_CASE(test_roundtrip_plain_ascii_and_crlf_lf) {
    std::string text = "Line 1 with CRLF\r\nLine 2 with LF\nLine 3 with Tab\tAnd Special Symbols: !@#$%^&*()_+\r\n";
    std::vector<uint8_t> data(text.begin(), text.end());
    verifyRoundTrip(data, "ascii_endings.txt");
}

TEST_CASE(test_roundtrip_unicode_scripts_and_bom) {
    // UTF-8 BOM: 0xEF, 0xBB, 0xBF
    std::vector<uint8_t> unicodeData = {0xEF, 0xBB, 0xBF};
    
    // Bengali: "আমার সোনার বাংলা"
    std::string bengali = "আমার সোনার বাংলা, আমি তোমায় ভালোবাসি। ";
    unicodeData.insert(unicodeData.end(), bengali.begin(), bengali.end());

    // Chinese: "你好世界"
    std::string chinese = "你好世界，霍夫曼编码测试。 ";
    unicodeData.insert(unicodeData.end(), chinese.begin(), chinese.end());

    // Accented European: "Café, résumé, Übergrößen, España"
    std::string accented = "Café, résumé, Übergrößen, España. ";
    unicodeData.insert(unicodeData.end(), accented.begin(), accented.end());

    // Emojis: 🚀🔥🎉💻
    std::string emojis = "🚀🔥🎉💻";
    unicodeData.insert(unicodeData.end(), emojis.begin(), emojis.end());

    verifyRoundTrip(unicodeData, "multilingual_bom.txt");
}

TEST_CASE(test_roundtrip_invalid_utf8_and_binary_data) {
    // Malformed UTF-8 sequences (isolated continuation bytes, truncated multibyte headers)
    std::vector<uint8_t> invalidUtf8 = {0xFF, 0xFE, 0x80, 0xBF, 0xE0, 0x80, 0xF0, 0x28, 0x00, 0x1F};
    verifyRoundTrip(invalidUtf8, "corrupt_utf8.bin");
}

TEST_CASE(test_roundtrip_random_bytes_and_stored_fallback) {
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> randomData(10000);
    for (size_t i = 0; i < randomData.size(); ++i) {
        randomData[i] = static_cast<uint8_t>(dist(gen));
    }

    // Standard Huffman round trip
    verifyRoundTrip(randomData, "random.bin");

    // Stored Fallback round trip on small incompressible data (where header makes Huffman larger)
    std::vector<uint8_t> smallIncompressible(100);
    for (size_t i = 0; i < 100; ++i) smallIncompressible[i] = static_cast<uint8_t>(dist(gen));

    MemoryInputStream srcStream(smallIncompressible);
    MemoryOutputStream arcStream;
    auto compRes = Compressor::compress(srcStream, arcStream, "small_stored.bin", 0, /*allowStoredFallback=*/true);
    ASSERT_TRUE(compRes.isOk);
    ASSERT_TRUE(compRes.value.usedStoredFallback);

    MemoryInputStream arcIn(arcStream.getBuffer());
    MemoryOutputStream restored;
    auto decompRes = Decompressor::decompress(arcIn, restored);
    ASSERT_TRUE(decompRes.isOk);
    ASSERT_TRUE(restored.getBuffer() == smallIncompressible);
}
