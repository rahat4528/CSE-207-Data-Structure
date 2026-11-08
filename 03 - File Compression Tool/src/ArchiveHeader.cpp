#include "ArchiveHeader.h"
#include <cstring>

namespace HuffmanEngine {

namespace {

inline void writeLE16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void writeLE32(std::vector<uint8_t>& buf, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

inline void writeLE64(std::vector<uint8_t>& buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

inline bool readLE16(IInputStream& stream, uint16_t& out) {
    uint8_t b[2];
    if (stream.read(b, 2) != 2) return false;
    out = static_cast<uint16_t>(b[0]) | (static_cast<uint16_t>(b[1]) << 8);
    return true;
}

inline bool readLE32(IInputStream& stream, uint32_t& out) {
    uint8_t b[4];
    if (stream.read(b, 4) != 4) return false;
    out = static_cast<uint32_t>(b[0]) |
          (static_cast<uint32_t>(b[1]) << 8) |
          (static_cast<uint32_t>(b[2]) << 16) |
          (static_cast<uint32_t>(b[3]) << 24);
    return true;
}

inline bool readLE64(IInputStream& stream, uint64_t& out) {
    uint8_t b[8];
    if (stream.read(b, 8) != 8) return false;
    out = 0;
    for (int i = 0; i < 8; ++i) {
        out |= (static_cast<uint64_t>(b[i]) << (i * 8));
    }
    return true;
}

} // anonymous namespace

Result<void> ArchiveHeader::validate() const {
    if (magic[0] != MAGIC_SIG[0] || magic[1] != MAGIC_SIG[1] ||
        magic[2] != MAGIC_SIG[2] || magic[3] != MAGIC_SIG[3]) {
        return Result<void>::fail(ErrorCode::ERR_INVALID_SIGNATURE, "Invalid magic signature in archive header");
    }

    if (version != FORMAT_VERSION_1) {
        return Result<void>::fail(ErrorCode::ERR_UNSUPPORTED_VERSION, "Unsupported archive format version: " + std::to_string(version));
    }

    if (reserved != 0) {
        return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Reserved flags must be zero");
    }

    if (alphabetSize > ALPHABET_SIZE) {
        return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Alphabet size exceeds maximum allowable limit (256)");
    }

    if (paddingBits > 7) {
        return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Padding bits must be between 0 and 7");
    }

    // Invariant: sum of frequencies must equal original size
    if (method == CompressionMethod::HUFFMAN && originalSize > 0) {
        uint64_t sumFreq = 0;
        for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
            sumFreq += frequencies[i];
        }
        if (sumFreq != originalSize) {
            return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Sum of symbol frequencies does not match declared original size");
        }
    }

    return Result<void>::ok();
}

Result<void> ArchiveHeader::writeToStream(IOutputStream& stream) const {
    std::vector<uint8_t> headerBytes;
    headerBytes.reserve(128 + alphabetSize * 9);

    // 1. Magic (4 bytes)
    headerBytes.insert(headerBytes.end(), magic, magic + 4);

    // 2. Version (1 byte)
    headerBytes.push_back(version);

    // 3. Method (1 byte)
    headerBytes.push_back(static_cast<uint8_t>(method));

    // 4. Reserved (2 bytes)
    writeLE16(headerBytes, reserved);

    // 5. Original Size (8 bytes)
    writeLE64(headerBytes, originalSize);

    // 6. Modification Time (8 bytes)
    writeLE64(headerBytes, modificationTime);

    // 7. Filename (2 bytes length + bytes)
    uint16_t nameLen = static_cast<uint16_t>(originalFilename.size());
    writeLE16(headerBytes, nameLen);
    if (nameLen > 0) {
        headerBytes.insert(headerBytes.end(), originalFilename.begin(), originalFilename.end());
    }

    // 8. Alphabet Size (2 bytes)
    writeLE16(headerBytes, alphabetSize);

    // 9. Active symbol frequencies
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (frequencies[i] > 0) {
            headerBytes.push_back(static_cast<uint8_t>(i));
            writeLE64(headerBytes, frequencies[i]);
        }
    }

    // 10. Padding bits (1 byte)
    headerBytes.push_back(paddingBits);

    // 11. Header CRC32 (4 bytes computed over all previous header bytes)
    uint32_t crc = CRC32::calculate(headerBytes.data(), headerBytes.size());
    writeLE32(headerBytes, crc);

    // Write all header bytes to output stream
    size_t written = stream.write(headerBytes.data(), headerBytes.size());
    if (written != headerBytes.size()) {
        return Result<void>::fail(ErrorCode::ERR_FILE_WRITE_FAILED, "Failed to write complete header to stream");
    }

    return Result<void>::ok();
}

Result<void> ArchiveHeader::readFromStream(IInputStream& stream) {
    std::vector<uint8_t> headerBytes;

    // 1. Magic
    if (stream.read(magic, 4) != 4) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive: unable to read magic signature");
    }
    headerBytes.insert(headerBytes.end(), magic, magic + 4);

    if (magic[0] != MAGIC_SIG[0] || magic[1] != MAGIC_SIG[1] ||
        magic[2] != MAGIC_SIG[2] || magic[3] != MAGIC_SIG[3]) {
        return Result<void>::fail(ErrorCode::ERR_INVALID_SIGNATURE, "Invalid magic signature in archive header");
    }

    // 2. Version
    if (stream.read(&version, 1) != 1) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing version");
    }
    headerBytes.push_back(version);

    if (version != FORMAT_VERSION_1) {
        return Result<void>::fail(ErrorCode::ERR_UNSUPPORTED_VERSION, "Unsupported archive format version: " + std::to_string(version));
    }

    // 3. Method
    uint8_t methodByte = 0;
    if (stream.read(&methodByte, 1) != 1) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing method");
    }
    method = static_cast<CompressionMethod>(methodByte);
    headerBytes.push_back(methodByte);

    // 4. Reserved
    if (!readLE16(stream, reserved)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing reserved flags");
    }
    writeLE16(headerBytes, reserved);
    if (reserved != 0) {
        return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Reserved flags must be zero");
    }

    // 5. Original Size
    if (!readLE64(stream, originalSize)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing original size");
    }
    writeLE64(headerBytes, originalSize);

    // 6. Modification Time
    if (!readLE64(stream, modificationTime)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing modification time");
    }
    writeLE64(headerBytes, modificationTime);

    // 7. Filename
    uint16_t nameLen = 0;
    if (!readLE16(stream, nameLen)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing name length");
    }
    writeLE16(headerBytes, nameLen);

    if (nameLen > 1024) {
        return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Filename length exceeds security limit (1024 bytes)");
    }

    if (nameLen > 0) {
        std::vector<uint8_t> nameBuf(nameLen);
        if (stream.read(nameBuf.data(), nameLen) != nameLen) {
            return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing filename data");
        }
        originalFilename.assign(nameBuf.begin(), nameBuf.end());
        headerBytes.insert(headerBytes.end(), nameBuf.begin(), nameBuf.end());
    } else {
        originalFilename.clear();
    }

    // 8. Alphabet Size
    if (!readLE16(stream, alphabetSize)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing alphabet size");
    }
    writeLE16(headerBytes, alphabetSize);

    if (alphabetSize > ALPHABET_SIZE) {
        return Result<void>::fail(ErrorCode::ERR_CORRUPTED_HEADER, "Alphabet size exceeds maximum allowable limit (256)");
    }

    // 9. Active symbol frequencies
    frequencies.fill(0);
    for (uint16_t i = 0; i < alphabetSize; ++i) {
        uint8_t sym = 0;
        if (stream.read(&sym, 1) != 1) {
            return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing symbol index");
        }
        headerBytes.push_back(sym);

        uint64_t freq = 0;
        if (!readLE64(stream, freq)) {
            return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing frequency data");
        }
        writeLE64(headerBytes, freq);
        frequencies[sym] = freq;
    }

    // 10. Padding bits
    if (stream.read(&paddingBits, 1) != 1) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing padding bits");
    }
    headerBytes.push_back(paddingBits);

    // 11. Read Header CRC32
    if (!readLE32(stream, headerCrc)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive header: missing header CRC32");
    }

    // Validate Header CRC32
    uint32_t expectedCrc = CRC32::calculate(headerBytes.data(), headerBytes.size());
    if (headerCrc != expectedCrc) {
        return Result<void>::fail(ErrorCode::ERR_HEADER_CRC_MISMATCH, "Header CRC32 mismatch: corrupted header metadata");
    }

    // Invariant validation
    return validate();
}

Result<void> ArchiveTrailer::writeToStream(IOutputStream& stream) const {
    std::vector<uint8_t> trailerBytes;
    writeLE32(trailerBytes, dataCrc);
    if (stream.write(trailerBytes.data(), 4) != 4) {
        return Result<void>::fail(ErrorCode::ERR_FILE_WRITE_FAILED, "Failed to write trailer CRC to stream");
    }
    return Result<void>::ok();
}

Result<void> ArchiveTrailer::readFromStream(IInputStream& stream) {
    if (!readLE32(stream, dataCrc)) {
        return Result<void>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Truncated archive: missing trailer CRC32");
    }
    return Result<void>::ok();
}

} // namespace HuffmanEngine
