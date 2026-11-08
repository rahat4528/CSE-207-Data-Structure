#ifndef HUFFMAN_PATH_SANITIZER_H
#define HUFFMAN_PATH_SANITIZER_H

#include "Common.h"
#include <filesystem>
#include <string>

namespace HuffmanEngine {

class PathSanitizer {
public:
    // Strips path traversal, drive letters, and Windows reserved names from stored metadata filenames
    static std::string sanitizeFilename(const std::string& rawName);

    // Resolves output path and ensures it stays strictly within the intended target directory
    static Result<std::filesystem::path> resolveSafeOutputPath(
        const std::filesystem::path& baseDir,
        const std::string& declaredFilename);

    // Validates that input file exists and is a readable regular file
    static Result<void> validateInputFile(const std::filesystem::path& inputPath);

    // Validates that output path does not overwrite input, and checks existence against force flag
    static Result<void> validateOutputFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        bool forceOverwrite);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_PATH_SANITIZER_H
