#include "BatchProcessor.h"
#include "PathSanitizer.h"
#include "FileIO.h"
#include <chrono>

namespace HuffmanEngine {

std::vector<std::filesystem::path> BatchProcessor::discoverFiles(
    const std::filesystem::path& dir,
    bool recursive) {

    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return files;
    }

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    }
    return files;
}

BatchSummary BatchProcessor::compressDirectory(
    const std::filesystem::path& inputDir,
    const std::filesystem::path& outputDir,
    bool recursive,
    bool forceOverwrite) {

    auto startTime = std::chrono::high_resolution_clock::now();
    BatchSummary summary;
    std::filesystem::create_directories(outputDir);

    auto files = discoverFiles(inputDir, recursive);
    summary.totalFiles = files.size();

    for (const auto& file : files) {
        BatchItemResult item;
        item.inputPath = file;
        item.outputPath = outputDir / (file.filename().string() + ".huff");

        // Validate paths
        auto valRes = PathSanitizer::validateOutputFile(item.inputPath, item.outputPath, forceOverwrite);
        if (!valRes.isOk) {
            item.success = false;
            item.errorMessage = valRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        FileInputStream inStream(item.inputPath);
        if (!inStream.isOpen()) {
            item.success = false;
            item.errorMessage = "Failed to open input file";
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        FileOutputStream outStream(item.outputPath, /*atomicTemp=*/true);
        if (!outStream.isOpen()) {
            item.success = false;
            item.errorMessage = "Failed to open output file for writing";
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        auto compRes = Compressor::compress(inStream, outStream, file.filename().string());
        if (!compRes.isOk) {
            outStream.abort();
            item.success = false;
            item.errorMessage = compRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        auto commitRes = outStream.commit();
        if (!commitRes.isOk) {
            item.success = false;
            item.errorMessage = commitRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        item.success = true;
        item.originalSize = compRes.value.originalSize;
        item.compressedSize = compRes.value.compressedSize;
        item.spaceSavings = compRes.value.spaceSavings;

        summary.successfulFiles++;
        summary.totalOriginalBytes += item.originalSize;
        summary.totalCompressedBytes += item.compressedSize;
        summary.items.push_back(item);
    }

    if (summary.totalOriginalBytes > 0) {
        double ratio = static_cast<double>(summary.totalCompressedBytes) / static_cast<double>(summary.totalOriginalBytes);
        summary.overallSavings = (1.0 - ratio) * 100.0;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    summary.totalTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    return summary;
}

BatchSummary BatchProcessor::decompressDirectory(
    const std::filesystem::path& inputDir,
    const std::filesystem::path& outputDir,
    bool recursive,
    bool forceOverwrite) {

    auto startTime = std::chrono::high_resolution_clock::now();
    BatchSummary summary;
    std::filesystem::create_directories(outputDir);

    auto files = discoverFiles(inputDir, recursive);
    for (const auto& file : files) {
        if (file.extension() != ".huff") continue; // Process only .huff files
        summary.totalFiles++;

        BatchItemResult item;
        item.inputPath = file;

        // Inspect header to extract original filename
        FileInputStream peekIn(file);
        ArchiveHeader header;
        auto hRes = header.readFromStream(peekIn);
        peekIn.close();

        if (!hRes.isOk) {
            item.success = false;
            item.errorMessage = "Malformed header: " + hRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        auto safeOutRes = PathSanitizer::resolveSafeOutputPath(outputDir, header.originalFilename);
        if (!safeOutRes.isOk) {
            item.success = false;
            item.errorMessage = safeOutRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }
        item.outputPath = safeOutRes.value;

        auto valRes = PathSanitizer::validateOutputFile(item.inputPath, item.outputPath, forceOverwrite);
        if (!valRes.isOk) {
            item.success = false;
            item.errorMessage = valRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        FileInputStream inStream(item.inputPath);
        FileOutputStream outStream(item.outputPath, /*atomicTemp=*/true);

        auto decompRes = Decompressor::decompress(inStream, outStream);
        if (!decompRes.isOk) {
            outStream.abort();
            item.success = false;
            item.errorMessage = decompRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        auto commitRes = outStream.commit();
        if (!commitRes.isOk) {
            item.success = false;
            item.errorMessage = commitRes.message;
            summary.failedFiles++;
            summary.items.push_back(item);
            continue;
        }

        item.success = true;
        item.originalSize = decompRes.value.decompressedSize;
        item.compressedSize = std::filesystem::file_size(item.inputPath);

        summary.successfulFiles++;
        summary.totalOriginalBytes += item.originalSize;
        summary.totalCompressedBytes += item.compressedSize;
        summary.items.push_back(item);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    summary.totalTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    return summary;
}

} // namespace HuffmanEngine
