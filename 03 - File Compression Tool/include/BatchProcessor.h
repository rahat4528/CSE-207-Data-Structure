#ifndef HUFFMAN_BATCH_PROCESSOR_H
#define HUFFMAN_BATCH_PROCESSOR_H

#include "Common.h"
#include "Compressor.h"
#include "Decompressor.h"
#include <filesystem>
#include <vector>
#include <string>

namespace HuffmanEngine {

struct BatchItemResult {
    std::filesystem::path inputPath;
    std::filesystem::path outputPath;
    bool success{false};
    filesize_t originalSize{0};
    filesize_t compressedSize{0};
    double spaceSavings{0.0};
    std::string errorMessage;
};

struct BatchSummary {
    size_t totalFiles{0};
    size_t successfulFiles{0};
    size_t failedFiles{0};
    filesize_t totalOriginalBytes{0};
    filesize_t totalCompressedBytes{0};
    double overallSavings{0.0};
    double totalTimeMs{0.0};
    std::vector<BatchItemResult> items;
};

class BatchProcessor {
public:
    // Discovers files in a directory (optionally recursively)
    static std::vector<std::filesystem::path> discoverFiles(
        const std::filesystem::path& dir,
        bool recursive = false);

    // Compresses all discovered files into independent .huff archives
    static BatchSummary compressDirectory(
        const std::filesystem::path& inputDir,
        const std::filesystem::path& outputDir,
        bool recursive = false,
        bool forceOverwrite = false);

    // Decompresses all .huff archives in a directory
    static BatchSummary decompressDirectory(
        const std::filesystem::path& inputDir,
        const std::filesystem::path& outputDir,
        bool recursive = false,
        bool forceOverwrite = false);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_BATCH_PROCESSOR_H
