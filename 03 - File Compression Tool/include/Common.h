#ifndef HUFFMAN_COMMON_H
#define HUFFMAN_COMMON_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <optional>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace HuffmanEngine {

using byte_t = uint8_t;
using frequency_t = uint64_t;
using filesize_t = uint64_t;

// Maximum distinct symbols for byte-level alphabet
constexpr size_t ALPHABET_SIZE = 256;
constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1024; // 64 KB default streaming buffer

// Binary Archive Magic Signature: "HUFF" (0x46465548 little endian / ASCII 'H','U','F','F')
constexpr uint8_t MAGIC_SIG[4] = {'H', 'U', 'F', 'F'};
constexpr uint8_t FORMAT_VERSION_1 = 0x01;

// Compression methods
enum class CompressionMethod : uint8_t {
    HUFFMAN = 0x00,
    STORED_RAW = 0x01
};

// Error Codes
enum class ErrorCode {
    SUCCESS = 0,
    ERR_FILE_NOT_FOUND,
    ERR_FILE_READ_FAILED,
    ERR_FILE_WRITE_FAILED,
    ERR_FILE_EXISTS,
    ERR_INVALID_SIGNATURE,
    ERR_UNSUPPORTED_VERSION,
    ERR_CORRUPTED_HEADER,
    ERR_HEADER_CRC_MISMATCH,
    ERR_DATA_CRC_MISMATCH,
    ERR_TRUNCATED_PAYLOAD,
    ERR_MALFORMED_BITSTREAM,
    ERR_PATH_TRAVERSAL,
    ERR_INVALID_PARAMETER,
    ERR_DECOMPRESSION_BOMB,
    ERR_INTERNAL
};

inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::ERR_FILE_NOT_FOUND: return "ERR_FILE_NOT_FOUND";
        case ErrorCode::ERR_FILE_READ_FAILED: return "ERR_FILE_READ_FAILED";
        case ErrorCode::ERR_FILE_WRITE_FAILED: return "ERR_FILE_WRITE_FAILED";
        case ErrorCode::ERR_FILE_EXISTS: return "ERR_FILE_EXISTS";
        case ErrorCode::ERR_INVALID_SIGNATURE: return "ERR_INVALID_SIGNATURE";
        case ErrorCode::ERR_UNSUPPORTED_VERSION: return "ERR_UNSUPPORTED_VERSION";
        case ErrorCode::ERR_CORRUPTED_HEADER: return "ERR_CORRUPTED_HEADER";
        case ErrorCode::ERR_HEADER_CRC_MISMATCH: return "ERR_HEADER_CRC_MISMATCH";
        case ErrorCode::ERR_DATA_CRC_MISMATCH: return "ERR_DATA_CRC_MISMATCH";
        case ErrorCode::ERR_TRUNCATED_PAYLOAD: return "ERR_TRUNCATED_PAYLOAD";
        case ErrorCode::ERR_MALFORMED_BITSTREAM: return "ERR_MALFORMED_BITSTREAM";
        case ErrorCode::ERR_PATH_TRAVERSAL: return "ERR_PATH_TRAVERSAL";
        case ErrorCode::ERR_INVALID_PARAMETER: return "ERR_INVALID_PARAMETER";
        case ErrorCode::ERR_DECOMPRESSION_BOMB: return "ERR_DECOMPRESSION_BOMB";
        case ErrorCode::ERR_INTERNAL: return "ERR_INTERNAL";
        default: return "UNKNOWN_ERROR";
    }
}

inline std::ostream& operator<<(std::ostream& os, ErrorCode code) {
    os << errorCodeToString(code);
    return os;
}

// Result monad
template <typename T>
class Result {
public:
    bool isOk;
    T value;
    ErrorCode error;
    std::string message;

    static Result<T> ok(T val) {
        Result<T> r;
        r.isOk = true;
        r.value = std::move(val);
        r.error = ErrorCode::SUCCESS;
        return r;
    }

    static Result<T> fail(ErrorCode err, std::string msg = "") {
        Result<T> r;
        r.isOk = false;
        r.error = err;
        r.message = std::move(msg);
        return r;
    }
};

template <>
class Result<void> {
public:
    bool isOk{false};
    ErrorCode error{ErrorCode::SUCCESS};
    std::string message;

    static Result<void> ok() {
        Result<void> r;
        r.isOk = true;
        r.error = ErrorCode::SUCCESS;
        return r;
    }

    static Result<void> fail(ErrorCode err, std::string msg = "") {
        Result<void> r;
        r.isOk = false;
        r.error = err;
        r.message = std::move(msg);
        return r;
    }
};

// BitCode representation capable of holding up to 256 bits
// (The theoretical maximum tree depth for 256 distinct symbols is 255)
class BitCode {
public:
    std::array<uint64_t, 4> words{0, 0, 0, 0}; // 256 bits total
    uint16_t length{0};                         // Bit count (0 to 256)

    BitCode() = default;

    void appendBit(bool bit) {
        if (length >= 256) return;
        if (bit) {
            size_t wordIdx = length / 64;
            size_t bitOffset = length % 64;
            words[wordIdx] |= (1ULL << bitOffset);
        }
        length++;
    }

    void popBit() {
        if (length == 0) return;
        length--;
        size_t wordIdx = length / 64;
        size_t bitOffset = length % 64;
        words[wordIdx] &= ~(1ULL << bitOffset);
    }

    bool getBit(size_t index) const {
        if (index >= length) return false;
        size_t wordIdx = index / 64;
        size_t bitOffset = index % 64;
        return (words[wordIdx] & (1ULL << bitOffset)) != 0;
    }

    std::string toString() const {
        std::string s;
        s.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            s.push_back(getBit(i) ? '1' : '0');
        }
        return s;
    }

    bool operator==(const BitCode& other) const {
        if (length != other.length) return false;
        size_t fullWords = length / 64;
        for (size_t i = 0; i < fullWords; ++i) {
            if (words[i] != other.words[i]) return false;
        }
        size_t remBits = length % 64;
        if (remBits > 0) {
            uint64_t mask = (1ULL << remBits) - 1ULL;
            if ((words[fullWords] & mask) != (other.words[fullWords] & mask)) return false;
        }
        return true;
    }
};

// IEEE 802.3 Standard CRC32 Engine (Table-driven)
class CRC32 {
public:
    static uint32_t calculate(const uint8_t* data, size_t length, uint32_t initial = 0xFFFFFFFF) {
        uint32_t crc = initial;
        for (size_t i = 0; i < length; ++i) {
            uint8_t tableIndex = static_cast<uint8_t>((crc ^ data[i]) & 0xFF);
            crc = (crc >> 8) ^ table[tableIndex];
        }
        return crc ^ 0xFFFFFFFF;
    }

    static uint32_t update(uint32_t crc, const uint8_t* data, size_t length) {
        uint32_t current = crc ^ 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            uint8_t tableIndex = static_cast<uint8_t>((current ^ data[i]) & 0xFF);
            current = (current >> 8) ^ table[tableIndex];
        }
        return current ^ 0xFFFFFFFF;
    }

private:
    static const uint32_t table[256];
};

} // namespace HuffmanEngine

#endif // HUFFMAN_COMMON_H
