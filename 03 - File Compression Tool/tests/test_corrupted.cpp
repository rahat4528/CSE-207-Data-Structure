#include "TestRunner.h"
#include "Compressor.h"
#include "Decompressor.h"
#include "FileIO.h"
#include <random>

using namespace HuffmanEngine;

TEST_CASE(test_corrupted_bit_flip_in_payload_detected) {
    std::string sample = "High reliability data integrity test payload with genuine Huffman compression.";
    std::vector<uint8_t> original(sample.begin(), sample.end());

    MemoryInputStream in(original);
    MemoryOutputStream arcOut;
    auto compRes = Compressor::compress(in, arcOut, "safe.txt");
    ASSERT_TRUE(compRes.isOk);

    auto validArchive = arcOut.takeBuffer();
    ASSERT_GT(validArchive.size(), 20);

    // Flip a bit in the payload section (after the header)
    size_t flipPos = validArchive.size() - 8;
    validArchive[flipPos] ^= 0x04;

    MemoryInputStream corruptedIn(validArchive);
    MemoryOutputStream targetOut;
    auto decompRes = Decompressor::decompress(corruptedIn, targetOut);

    // Must fail cleanly with CRC mismatch or malformed bitstream
    ASSERT_FALSE(decompRes.isOk);
    ASSERT_TRUE(decompRes.error == ErrorCode::ERR_DATA_CRC_MISMATCH ||
                decompRes.error == ErrorCode::ERR_MALFORMED_BITSTREAM);
}

TEST_CASE(test_corrupted_disk_temp_cleanup_on_failure) {
    std::filesystem::path testDir = "test_out";
    std::filesystem::create_directories(testDir);
    std::filesystem::path badArcPath = testDir / "corrupted_archive.huff";
    std::filesystem::path finalOutPath = testDir / "should_not_exist.txt";

    std::filesystem::remove(badArcPath);
    std::filesystem::remove(finalOutPath);

    // Create a truncated/corrupted archive file on disk
    std::vector<uint8_t> junk = {'H', 'U', 'F', 'F', 0x01, 0x00, 0x00, 0x00, 0x50, 0x00};
    {
        std::ofstream f(badArcPath, std::ios::binary);
        f.write(reinterpret_cast<const char*>(junk.data()), junk.size());
    }

    {
        FileInputStream arcIn(badArcPath);
        FileOutputStream fileOut(finalOutPath, /*atomicTemp=*/true);

        auto decompRes = Decompressor::decompress(arcIn, fileOut);
        ASSERT_FALSE(decompRes.isOk);

        // Crucial safety rule: on failure, abort() and ensure final target file does NOT exist!
        fileOut.abort();
    }

    ASSERT_FALSE(std::filesystem::exists(finalOutPath));
    std::filesystem::remove(badArcPath);
}

TEST_CASE(test_decompression_bomb_rejection) {
    ArchiveHeader bombHeader;
    bombHeader.originalSize = 50ULL * 1024ULL * 1024ULL * 1024ULL; // 50 GB
    bombHeader.originalFilename = "huge_bomb.bin";
    bombHeader.alphabetSize = 1;
    bombHeader.frequencies['A'] = bombHeader.originalSize;
    bombHeader.paddingBits = 0;

    MemoryOutputStream arcOut;
    bombHeader.writeToStream(arcOut);
    // Put tiny 4 bytes payload + 4 bytes trailer
    uint32_t payload = 0;
    arcOut.write(reinterpret_cast<const uint8_t*>(&payload), 4);
    ArchiveTrailer trailer{0};
    trailer.writeToStream(arcOut);

    MemoryInputStream arcIn(arcOut.getBuffer());
    MemoryOutputStream targetOut;

    // Default max cap is 10 GB, so 50 GB should be rejected immediately before allocation
    auto decompRes = Decompressor::decompress(arcIn, targetOut);
    ASSERT_FALSE(decompRes.isOk);
    ASSERT_EQ(decompRes.error, ErrorCode::ERR_DECOMPRESSION_BOMB);
}

TEST_CASE(test_fuzz_mutation_robustness) {
    // Generate valid archive
    std::string text = "Mutation fuzzing suite: ensuring no crashes, infinite loops, or memory errors.";
    std::vector<uint8_t> data(text.begin(), text.end());

    MemoryInputStream in(data);
    MemoryOutputStream arcOut;
    Compressor::compress(in, arcOut, "fuzz_base.txt");
    const auto validBytes = arcOut.getBuffer();

    std::mt19937 rng(1337);
    std::uniform_int_distribution<size_t> posDist(0, validBytes.size() - 1);
    std::uniform_int_distribution<int> valDist(0, 255);
    std::uniform_int_distribution<int> actionDist(0, 2);

    constexpr int MUTATION_ROUNDS = 500;
    for (int round = 0; round < MUTATION_ROUNDS; ++round) {
        auto mutated = validBytes;
        int action = actionDist(rng);
        if (action == 0 && !mutated.empty()) {
            // Bit flip or byte corrupt
            size_t p = posDist(rng);
            mutated[p] ^= static_cast<uint8_t>(valDist(rng) | 1);
        } else if (action == 1) {
            // Truncate at random position
            size_t newSize = rng() % mutated.size();
            mutated.resize(newSize);
        } else {
            // Append random trailing garbage
            mutated.push_back(static_cast<uint8_t>(valDist(rng)));
        }

        MemoryInputStream mutIn(mutated);
        MemoryOutputStream mutOut;

        // Must NEVER crash or throw unhandled exceptions
        try {
            auto res = Decompressor::decompress(mutIn, mutOut);
            // If it succeeded, output must match original; otherwise clean error code
            if (res.isOk) {
                ASSERT_TRUE(mutOut.getBuffer() == data);
            } else {
                ASSERT_NE(res.error, ErrorCode::SUCCESS);
            }
        } catch (...) {
            ASSERT_TRUE(false); // No unhandled exception allowed!
        }
    }
}
