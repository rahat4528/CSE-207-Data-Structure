#ifndef HUFFMAN_DECOMPRESSOR_H
#define HUFFMAN_DECOMPRESSOR_H

#include "Common.h"
#include "IStream.h"
#include "ArchiveHeader.h"
#include "HuffmanTree.h"
#include "BitBuffer.h"

namespace HuffmanEngine {

struct DecompressionStats {
    filesize_t decompressedSize{0};
    filesize_t archiveSize{0};
    uint32_t dataCrc{0};
    double elapsedTimeMs{0.0};
    double throughputMBs{0.0};
    std::string originalFilename;
    uint64_t modificationTime{0};
};

class Decompressor {
private:
    static constexpr filesize_t MAX_DECOMPRESSION_SIZE = 10ULL * 1024ULL * 1024ULL * 1024ULL; // 10 GB bomb safety cap

public:
    static Result<DecompressionStats> decompress(
        IInputStream& archiveStream,
        IOutputStream& targetStream,
        filesize_t maxAllowedSize = MAX_DECOMPRESSION_SIZE);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_DECOMPRESSOR_H
