#ifndef HUFFMAN_COMPRESSOR_H
#define HUFFMAN_COMPRESSOR_H

#include "Common.h"
#include "IStream.h"
#include "ArchiveHeader.h"
#include "FrequencyCounter.h"
#include "HuffmanTree.h"
#include "BitBuffer.h"

namespace HuffmanEngine {

struct CompressionStats {
    filesize_t originalSize{0};
    filesize_t compressedSize{0};
    filesize_t headerSize{0};
    filesize_t payloadSize{0};
    double compressionRatio{0.0};  // compressedSize / originalSize
    double spaceSavings{0.0};      // 1.0 - (compressedSize / originalSize)
    double entropy{0.0};           // Shannon entropy in bits/symbol
    double averageCodeLength{0.0}; // Average bits/symbol
    double efficiency{0.0};        // Entropy / averageCodeLength * 100%
    double elapsedTimeMs{0.0};
    double throughputMBs{0.0};
    bool usedStoredFallback{false};
};

class Compressor {
public:
    static Result<CompressionStats> compress(
        IInputStream& source,
        IOutputStream& targetArchive,
        const std::string& originalFilename = "",
        uint64_t modificationTime = 0,
        bool allowStoredFallback = false);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_COMPRESSOR_H
