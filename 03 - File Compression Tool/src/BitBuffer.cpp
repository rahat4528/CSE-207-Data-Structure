#include "BitBuffer.h"

namespace HuffmanEngine {

BitWriter::BitWriter(IOutputStream& stream) : outStream(stream) {}

BitWriter::~BitWriter() {
    flush();
}

void BitWriter::writeBit(bool bit) {
    accumulator = (accumulator << 1) | (bit ? 1ULL : 0ULL);
    bitCount++;
    totalBitsWritten++;

    if (bitCount == 8) {
        uint8_t byteVal = static_cast<uint8_t>(accumulator & 0xFF);
        outStream.write(&byteVal, 1);
        accumulator = 0;
        bitCount = 0;
        totalBytesEmitted++;
    }
}

void BitWriter::writeBits(uint64_t value, uint8_t count) {
    for (int i = static_cast<int>(count) - 1; i >= 0; --i) {
        bool bit = ((value >> i) & 1ULL) != 0;
        writeBit(bit);
    }
}

void BitWriter::writeCode(const BitCode& code) {
    for (size_t i = 0; i < code.length; ++i) {
        writeBit(code.getBit(i));
    }
}

uint8_t BitWriter::flush() {
    if (bitCount == 0) {
        outStream.flush();
        return 0;
    }

    uint8_t paddingBits = static_cast<uint8_t>(8 - bitCount);
    accumulator <<= paddingBits; // Zero-pad the remaining bits to the right
    uint8_t byteVal = static_cast<uint8_t>(accumulator & 0xFF);
    outStream.write(&byteVal, 1);
    totalBytesEmitted++;

    accumulator = 0;
    bitCount = 0;
    outStream.flush();

    return paddingBits;
}

// BitReader implementation
BitReader::BitReader(IInputStream& stream, uint64_t maxBits)
    : inStream(stream), maxBitsToRead(maxBits) {}

bool BitReader::fillBuffer() {
    if (eofReached) return false;

    // Buffer up to 56 bits in the 64-bit accumulator
    while (bitCount <= 56) {
        uint8_t byteVal = 0;
        size_t bytesRead = inStream.read(&byteVal, 1);
        if (bytesRead == 0) {
            eofReached = true;
            break;
        }
        accumulator = (accumulator << 8) | static_cast<uint64_t>(byteVal);
        bitCount = static_cast<uint8_t>(bitCount + 8);
    }
    return bitCount > 0;
}

std::optional<bool> BitReader::readBit() {
    if (bitsRead >= maxBitsToRead) {
        return std::nullopt;
    }

    if (bitCount == 0) {
        if (!fillBuffer()) {
            return std::nullopt;
        }
    }

    bool bit = ((accumulator >> (bitCount - 1)) & 1ULL) != 0;
    bitCount--;
    bitsRead++;
    return bit;
}

bool BitReader::readBits(uint8_t count, uint64_t& value) {
    value = 0;
    for (uint8_t i = 0; i < count; ++i) {
        auto b = readBit();
        if (!b.has_value()) return false;
        value = (value << 1) | (*b ? 1ULL : 0ULL);
    }
    return true;
}

} // namespace HuffmanEngine
