#include "TestRunner.h"
#include "Common.h"

// Include test suites
#include "test_frequency.cpp"
#include "test_heap.cpp"
#include "test_huffman_tree.cpp"
#include "test_bit_buffer.cpp"
#include "test_archive.cpp"
#include "test_roundtrip.cpp"
#include "test_streaming.cpp"
#include "test_corrupted.cpp"
#include "test_path_safety.cpp"
#include "test_batch_cli.cpp"

using namespace HuffmanEngine;

TEST_CASE(test_common_crc32_and_bitcode) {
    const std::string text = "123456789";
    uint32_t crc = CRC32::calculate(reinterpret_cast<const uint8_t*>(text.data()), text.size());
    // Standard test vector for "123456789" is 0xCBF43926
    ASSERT_EQ(crc, 0xCBF43926);

    BitCode code;
    ASSERT_EQ(code.length, 0);
    code.appendBit(true);
    code.appendBit(false);
    code.appendBit(true);
    code.appendBit(true);
    ASSERT_EQ(code.length, 4);
    ASSERT_EQ(code.toString(), "1011");
    ASSERT_TRUE(code.getBit(0));
    ASSERT_FALSE(code.getBit(1));
    ASSERT_TRUE(code.getBit(2));
    ASSERT_TRUE(code.getBit(3));

    code.popBit();
    ASSERT_EQ(code.length, 3);
    ASSERT_EQ(code.toString(), "101");
}

int main() {
    return MiniTest::runAllTests();
}
