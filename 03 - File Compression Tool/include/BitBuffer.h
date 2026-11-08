#ifndef HUFFMAN_BIT_BUFFER_H
#define HUFFMAN_BIT_BUFFER_H

#include "Common.h"
#include "IStream.h"
#include <vector>
#include <optional>

namespace HuffmanEngine {

// BitWriter packs variable-length binary codes into bytes (MSB first)
class BitWriter {
private:
    IOutputStream& outStream;
    uint64_t accumulator{0};
    uint8_t bitCount{0};
    uint64_t totalBitsWritten{0};
    uint64_t totalBytesEmitted{0};

public:
    explicit BitWriter(IOutputStream& stream);
    ~BitWriter();

    void writeBit(bool bit);
    void writeBits(uint64_t value, uint8_t count);
    void writeCode(const BitCode& code);

    // Flushes any remaining bits in the accumulator, zero-padded to a full byte
    // Returns the number of padding bits added (0 to 7)
    uint8_t flush();

    uint64_t getTotalBits() const { return totalBitsWritten; }
    uint64_t getBytesEmitted() const { return totalBytesEmitted; }
};

// BitReader un-packs variable-length binary bits from a byte stream (MSB first)
class BitReader {
private:
    IInputStream& inStream;
    uint64_t accumulator{0};
    uint8_t bitCount{0};
    uint64_t bitsRead{0};
    uint64_t maxBitsToRead{UINT64_MAX};
    bool eofReached{false};

    bool fillBuffer();

public:
    explicit BitReader(IInputStream& stream, uint64_t maxBits = UINT64_MAX);

    // Reads a single bit. Returns std::nullopt on EOF or bit limit reached
    std::optional<bool> readBit();

    // Reads up to 64 bits into value
    bool readBits(uint8_t count, uint64_t& value);

    uint64_t getBitsRead() const { return bitsRead; }
    bool isEof() const { return eofReached && (bitCount == 0); }
    void setMaxBits(uint64_t maxBits) { maxBitsToRead = maxBits; }
};

} // namespace HuffmanEngine

#endif // HUFFMAN_BIT_BUFFER_H
