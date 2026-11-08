#include "TestRunner.h"
#include "BitBuffer.h"
#include <vector>

using namespace HuffmanEngine;

TEST_CASE(test_bit_buffer_all_eight_paddings) {
    for (uint8_t k = 0; k <= 7; ++k) {
        // Write (16 + k) bits
        size_t numBits = 16 + k;
        MemoryOutputStream outStream;
        BitWriter writer(outStream);

        std::vector<bool> expectedBits;
        for (size_t b = 0; b < numBits; ++b) {
            bool bit = ((b % 3) == 0) || ((b % 5) == 0);
            expectedBits.push_back(bit);
            writer.writeBit(bit);
        }

        uint8_t padding = writer.flush();
        uint8_t expectedPadding = static_cast<uint8_t>((8 - (numBits % 8)) % 8);
        ASSERT_EQ(padding, expectedPadding);

        // Verify read back
        MemoryInputStream inStream(outStream.getBuffer());
        BitReader reader(inStream, numBits);

        for (size_t b = 0; b < numBits; ++b) {
            auto bitOpt = reader.readBit();
            ASSERT_TRUE(bitOpt.has_value());
            ASSERT_EQ(*bitOpt, expectedBits[b]);
        }

        // Subsequent read should hit maxBits limit
        ASSERT_FALSE(reader.readBit().has_value());
    }
}

TEST_CASE(test_bit_buffer_multibyte_arbitrary_codes) {
    MemoryOutputStream outStream;
    BitWriter writer(outStream);

    std::vector<BitCode> codes;
    // Create diverse codes of lengths 1, 3, 7, 12, 19, 25
    std::vector<uint16_t> lens = {1, 3, 7, 12, 19, 25, 5, 2, 8};
    for (size_t i = 0; i < lens.size(); ++i) {
        BitCode c;
        for (uint16_t j = 0; j < lens[i]; ++j) {
            c.appendBit((j + i) % 2 == 1);
        }
        codes.push_back(c);
        writer.writeCode(c);
    }
    writer.flush();
    uint64_t totalBits = writer.getTotalBits();

    MemoryInputStream inStream(outStream.getBuffer());
    BitReader reader(inStream, totalBits);

    for (size_t i = 0; i < codes.size(); ++i) {
        BitCode decoded;
        for (uint16_t j = 0; j < codes[i].length; ++j) {
            auto b = reader.readBit();
            ASSERT_TRUE(b.has_value());
            decoded.appendBit(*b);
        }
        ASSERT_EQ(decoded.length, codes[i].length);
        ASSERT_EQ(decoded.toString(), codes[i].toString());
    }
    ASSERT_EQ(reader.getBitsRead(), totalBits);
}

TEST_CASE(test_bit_buffer_multi_bit_integers) {
    MemoryOutputStream outStream;
    BitWriter writer(outStream);

    writer.writeBits(0x2A, 6);   // 42 in 6 bits
    writer.writeBits(0x3FF, 10); // 1023 in 10 bits
    writer.writeBits(0x05, 3);   // 5 in 3 bits
    writer.flush();

    MemoryInputStream inStream(outStream.getBuffer());
    BitReader reader(inStream, 6 + 10 + 3);

    uint64_t v1 = 0, v2 = 0, v3 = 0;
    ASSERT_TRUE(reader.readBits(6, v1));
    ASSERT_EQ(v1, 0x2A);

    ASSERT_TRUE(reader.readBits(10, v2));
    ASSERT_EQ(v2, 0x3FF);

    ASSERT_TRUE(reader.readBits(3, v3));
    ASSERT_EQ(v3, 0x05);
}
