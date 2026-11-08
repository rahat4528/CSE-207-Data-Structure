#include "TestRunner.h"
#include "ArchiveHeader.h"

using namespace HuffmanEngine;

TEST_CASE(test_archive_header_valid_roundtrip) {
    ArchiveHeader h1;
    h1.originalSize = 1048576;
    h1.modificationTime = 1730000000;
    h1.originalFilename = "sample_data.txt";
    h1.alphabetSize = 3;
    h1.frequencies['A'] = 500000;
    h1.frequencies['B'] = 300000;
    h1.frequencies['C'] = 248576;
    h1.paddingBits = 3;

    MemoryOutputStream outStream;
    auto writeRes = h1.writeToStream(outStream);
    ASSERT_TRUE(writeRes.isOk);

    MemoryInputStream inStream(outStream.getBuffer());
    ArchiveHeader h2;
    auto readRes = h2.readFromStream(inStream);
    ASSERT_TRUE(readRes.isOk);

    ASSERT_EQ(h2.originalSize, h1.originalSize);
    ASSERT_EQ(h2.modificationTime, h1.modificationTime);
    ASSERT_EQ(h2.originalFilename, h1.originalFilename);
    ASSERT_EQ(h2.alphabetSize, 3);
    ASSERT_EQ(h2.frequencies['A'], 500000);
    ASSERT_EQ(h2.frequencies['B'], 300000);
    ASSERT_EQ(h2.frequencies['C'], 248576);
    ASSERT_EQ(h2.paddingBits, 3);
}

TEST_CASE(test_archive_header_crc_mismatch_on_bit_flip) {
    ArchiveHeader h1;
    h1.originalSize = 100;
    h1.originalFilename = "test.txt";
    h1.alphabetSize = 1;
    h1.frequencies['A'] = 100;

    MemoryOutputStream outStream;
    h1.writeToStream(outStream);

    auto buf = outStream.takeBuffer();
    // Flip a bit in the filename or original size field
    buf[12] ^= 0x01;

    MemoryInputStream corruptedStream(buf);
    ArchiveHeader h2;
    auto readRes = h2.readFromStream(corruptedStream);
    ASSERT_FALSE(readRes.isOk);
    ASSERT_EQ(readRes.error, ErrorCode::ERR_HEADER_CRC_MISMATCH);
}

TEST_CASE(test_archive_header_invalid_magic_and_version) {
    std::vector<uint8_t> badMagic = {'B', 'A', 'D', '!', 0x01, 0x00, 0x00, 0x00};
    MemoryInputStream s1(badMagic);
    ArchiveHeader h1;
    auto res1 = h1.readFromStream(s1);
    ASSERT_FALSE(res1.isOk);
    ASSERT_EQ(res1.error, ErrorCode::ERR_INVALID_SIGNATURE);

    std::vector<uint8_t> badVersion = {'H', 'U', 'F', 'F', 0x99, 0x00, 0x00, 0x00};
    MemoryInputStream s2(badVersion);
    ArchiveHeader h2;
    auto res2 = h2.readFromStream(s2);
    ASSERT_FALSE(res2.isOk);
    ASSERT_EQ(res2.error, ErrorCode::ERR_UNSUPPORTED_VERSION);
}

TEST_CASE(test_archive_header_truncation_detection) {
    ArchiveHeader h1;
    h1.originalSize = 500;
    h1.originalFilename = "readme.md";
    h1.alphabetSize = 1;
    h1.frequencies['X'] = 500;

    MemoryOutputStream outStream;
    h1.writeToStream(outStream);

    auto buf = outStream.takeBuffer();
    // Truncate to just 15 bytes
    buf.resize(15);

    MemoryInputStream truncatedStream(buf);
    ArchiveHeader h2;
    auto res = h2.readFromStream(truncatedStream);
    ASSERT_FALSE(res.isOk);
    ASSERT_EQ(res.error, ErrorCode::ERR_TRUNCATED_PAYLOAD);
}
