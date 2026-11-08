#ifndef HUFFMAN_ARCHIVE_HEADER_H
#define HUFFMAN_ARCHIVE_HEADER_H

#include "Common.h"
#include "IStream.h"
#include <string>
#include <array>
#include <vector>

namespace HuffmanEngine {

struct ArchiveHeader {
    uint8_t magic[4]{'H', 'U', 'F', 'F'};
    uint8_t version{FORMAT_VERSION_1};
    CompressionMethod method{CompressionMethod::HUFFMAN};
    uint16_t reserved{0};
    filesize_t originalSize{0};
    uint64_t modificationTime{0};
    std::string originalFilename;
    uint16_t alphabetSize{0};
    std::array<frequency_t, ALPHABET_SIZE> frequencies{};
    uint8_t paddingBits{0};
    uint32_t headerCrc{0};

    ArchiveHeader() {
        frequencies.fill(0);
    }

    // Serializes header to output stream, calculates and appends header CRC32
    Result<void> writeToStream(IOutputStream& stream) const;

    // Deserializes and validates header from input stream
    Result<void> readFromStream(IInputStream& stream);

    // Validates header invariants (version, CRC, sum of frequencies, bounds)
    Result<void> validate() const;
};

// End-of-file trailer storing data integrity CRC32
struct ArchiveTrailer {
    uint32_t dataCrc{0};

    Result<void> writeToStream(IOutputStream& stream) const;
    Result<void> readFromStream(IInputStream& stream);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_ARCHIVE_HEADER_H
