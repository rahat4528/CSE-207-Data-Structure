#include "Compressor.h"
#include <chrono>
#include <vector>

namespace HuffmanEngine {

Result<CompressionStats> Compressor::compress(
    IInputStream& source,
    IOutputStream& targetArchive,
    const std::string& originalFilename,
    uint64_t modificationTime,
    bool allowStoredFallback) {

    auto startTime = std::chrono::high_resolution_clock::now();
    CompressionStats stats;
    filesize_t initialWritten = targetArchive.bytesWritten();

    // Pass 1: Frequency counting and preliminary metrics
    FrequencyCounter counter;
    auto countRes = counter.countStream(source);
    if (!countRes.isOk) {
        return Result<CompressionStats>::fail(countRes.error, countRes.message);
    }

    filesize_t origSize = counter.getTotalBytes();
    stats.originalSize = origSize;
    stats.entropy = counter.calculateEntropy();

    // Special Case 1: Empty file
    if (origSize == 0) {
        ArchiveHeader header;
        header.originalSize = 0;
        header.modificationTime = modificationTime;
        header.originalFilename = originalFilename;
        header.alphabetSize = 0;
        header.paddingBits = 0;
        header.method = CompressionMethod::HUFFMAN;

        auto hRes = header.writeToStream(targetArchive);
        if (!hRes.isOk) return Result<CompressionStats>::fail(hRes.error, hRes.message);

        ArchiveTrailer trailer;
        trailer.dataCrc = 0;
        auto tRes = trailer.writeToStream(targetArchive);
        if (!tRes.isOk) return Result<CompressionStats>::fail(tRes.error, tRes.message);

        auto endTime = std::chrono::high_resolution_clock::now();
        stats.elapsedTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        stats.compressedSize = targetArchive.bytesWritten() - initialWritten;
        stats.headerSize = stats.compressedSize - 4; // Minus 4 bytes trailer
        stats.payloadSize = 0;
        stats.compressionRatio = 0.0;
        stats.spaceSavings = 0.0;
        return Result<CompressionStats>::ok(stats);
    }

    // Build Huffman Tree
    HuffmanTree tree;
    auto treeRes = tree.buildFromFrequencies(counter.getTable());
    if (!treeRes.isOk) {
        return Result<CompressionStats>::fail(treeRes.error, treeRes.message);
    }

    stats.averageCodeLength = tree.calculateAverageCodeLength(counter.getTable(), origSize);
    if (stats.averageCodeLength > 0.0) {
        stats.efficiency = (stats.entropy / stats.averageCodeLength) * 100.0;
    }

    // Calculate total encoded bit length to compute exact padding bits in advance
    uint64_t totalPayloadBits = 0;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (counter.getFrequency(static_cast<byte_t>(i)) > 0) {
            totalPayloadBits += counter.getFrequency(static_cast<byte_t>(i)) * tree.getCode(static_cast<byte_t>(i)).length;
        }
    }
    uint8_t paddingBits = static_cast<uint8_t>((8 - (totalPayloadBits % 8)) % 8);
    uint64_t expectedPayloadBytes = (totalPayloadBits + paddingBits) / 8;

    // Check Stored Fallback condition: if total Huffman archive would be larger than the input
    filesize_t estimatedHeaderSize = 35 + (counter.getDistinctSymbolCount() * 9);
    filesize_t estimatedHuffmanTotal = estimatedHeaderSize + expectedPayloadBytes + 4;

    bool useStored = false;
    if (allowStoredFallback && (estimatedHuffmanTotal >= origSize)) {
        useStored = true;
    }

    // Prepare Header
    ArchiveHeader header;
    header.originalSize = origSize;
    header.modificationTime = modificationTime;
    header.originalFilename = originalFilename;
    header.method = useStored ? CompressionMethod::STORED_RAW : CompressionMethod::HUFFMAN;
    header.paddingBits = useStored ? 0 : paddingBits;
    header.alphabetSize = static_cast<uint16_t>(counter.getDistinctSymbolCount());
    header.frequencies = counter.getTable();

    filesize_t beforeHeader = targetArchive.bytesWritten();
    auto hRes = header.writeToStream(targetArchive);
    if (!hRes.isOk) return Result<CompressionStats>::fail(hRes.error, hRes.message);
    stats.headerSize = targetArchive.bytesWritten() - beforeHeader;

    // Pass 2: Stream encoding
    source.rewind();
    uint32_t runningCrc = 0xFFFFFFFF;
    filesize_t beforePayload = targetArchive.bytesWritten();

    if (useStored) {
        // Raw Stored Fallback
        std::vector<uint8_t> inBuf(DEFAULT_BUFFER_SIZE);
        while (!source.isEof()) {
            size_t bytesRead = source.read(inBuf.data(), DEFAULT_BUFFER_SIZE);
            if (bytesRead == 0) break;
            runningCrc = CRC32::update(runningCrc, inBuf.data(), bytesRead);
            targetArchive.write(inBuf.data(), bytesRead);
        }
        stats.usedStoredFallback = true;
    } else {
        // Standard Huffman bitstream encoding
        BitWriter writer(targetArchive);
        std::vector<uint8_t> inBuf(DEFAULT_BUFFER_SIZE);

        while (!source.isEof()) {
            size_t bytesRead = source.read(inBuf.data(), DEFAULT_BUFFER_SIZE);
            if (bytesRead == 0) break;
            runningCrc = CRC32::update(runningCrc, inBuf.data(), bytesRead);

            for (size_t b = 0; b < bytesRead; ++b) {
                byte_t sym = inBuf[b];
                writer.writeCode(tree.getCode(sym));
            }
        }
        uint8_t flushedPad = writer.flush();
        (void)flushedPad;
    }

    stats.payloadSize = targetArchive.bytesWritten() - beforePayload;

    // Write Archive Trailer
    ArchiveTrailer trailer;
    trailer.dataCrc = runningCrc ^ 0xFFFFFFFF;
    auto tRes = trailer.writeToStream(targetArchive);
    if (!tRes.isOk) return Result<CompressionStats>::fail(tRes.error, tRes.message);

    stats.compressedSize = targetArchive.bytesWritten() - initialWritten;
    stats.compressionRatio = static_cast<double>(stats.compressedSize) / static_cast<double>(origSize);
    stats.spaceSavings = (1.0 - stats.compressionRatio) * 100.0;

    auto endTime = std::chrono::high_resolution_clock::now();
    stats.elapsedTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    if (stats.elapsedTimeMs > 0.0) {
        double seconds = stats.elapsedTimeMs / 1000.0;
        stats.throughputMBs = (static_cast<double>(origSize) / (1024.0 * 1024.0)) / seconds;
    }

    return Result<CompressionStats>::ok(stats);
}

} // namespace HuffmanEngine
