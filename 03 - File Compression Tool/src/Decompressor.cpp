#include "Decompressor.h"
#include <chrono>
#include <vector>
#include <cstring>

namespace HuffmanEngine {

Result<DecompressionStats> Decompressor::decompress(
    IInputStream& archiveStream,
    IOutputStream& targetStream,
    filesize_t maxAllowedSize) {

    auto startTime = std::chrono::high_resolution_clock::now();
    DecompressionStats stats;
    filesize_t initialWritten = targetStream.bytesWritten();

    // 1. Read Archive Header
    ArchiveHeader header;
    auto hRes = header.readFromStream(archiveStream);
    if (!hRes.isOk) {
        return Result<DecompressionStats>::fail(hRes.error, hRes.message);
    }

    stats.originalFilename = header.originalFilename;
    stats.modificationTime = header.modificationTime;

    // 2. Safety Limit (Decompression Bomb Protection)
    if (header.originalSize > maxAllowedSize) {
        return Result<DecompressionStats>::fail(
            ErrorCode::ERR_DECOMPRESSION_BOMB,
            "Declared decompressed size (" + std::to_string(header.originalSize) +
            " bytes) exceeds safety maximum (" + std::to_string(maxAllowedSize) + " bytes)");
    }

    uint32_t runningCrc = 0xFFFFFFFF;

    // 3. Special Case: Empty file
    if (header.originalSize == 0) {
        ArchiveTrailer trailer;
        auto tRes = trailer.readFromStream(archiveStream);
        if (!tRes.isOk) {
            return Result<DecompressionStats>::fail(tRes.error, tRes.message);
        }
        if (trailer.dataCrc != 0) {
            return Result<DecompressionStats>::fail(ErrorCode::ERR_DATA_CRC_MISMATCH, "Non-zero CRC for empty file archive");
        }

        stats.decompressedSize = 0;
        stats.dataCrc = 0;
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.elapsedTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return Result<DecompressionStats>::ok(stats);
    }

    // 4. Stored Fallback Mode
    if (header.method == CompressionMethod::STORED_RAW) {
        std::vector<uint8_t> buffer(DEFAULT_BUFFER_SIZE);
        filesize_t remaining = header.originalSize;

        while (remaining > 0) {
            size_t toRead = (remaining < DEFAULT_BUFFER_SIZE) ? static_cast<size_t>(remaining) : DEFAULT_BUFFER_SIZE;
            size_t bytesRead = archiveStream.read(buffer.data(), toRead);
            if (bytesRead == 0) {
                return Result<DecompressionStats>::fail(ErrorCode::ERR_TRUNCATED_PAYLOAD, "Premature end of stored data payload");
            }
            runningCrc = CRC32::update(runningCrc, buffer.data(), bytesRead);
            targetStream.write(buffer.data(), bytesRead);
            remaining -= bytesRead;
        }

        ArchiveTrailer trailer;
        auto tRes = trailer.readFromStream(archiveStream);
        if (!tRes.isOk) return Result<DecompressionStats>::fail(tRes.error, tRes.message);

        uint32_t calculatedCrc = runningCrc ^ 0xFFFFFFFF;
        if (trailer.dataCrc != calculatedCrc) {
            return Result<DecompressionStats>::fail(ErrorCode::ERR_DATA_CRC_MISMATCH, "Data CRC32 mismatch on stored archive extraction");
        }

        stats.decompressedSize = targetStream.bytesWritten() - initialWritten;
        stats.dataCrc = calculatedCrc;
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.elapsedTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        if (stats.elapsedTimeMs > 0.0) {
            double sec = stats.elapsedTimeMs / 1000.0;
            stats.throughputMBs = (static_cast<double>(stats.decompressedSize) / (1024.0 * 1024.0)) / sec;
        }
        return Result<DecompressionStats>::ok(stats);
    }

    // 5. Standard Huffman Decompression
    HuffmanTree tree;
    auto treeRes = tree.buildFromFrequencies(header.frequencies);
    if (!treeRes.isOk) {
        return Result<DecompressionStats>::fail(treeRes.error, treeRes.message);
    }

    const HuffmanNode* root = tree.getRoot();
    if (!root) {
        return Result<DecompressionStats>::fail(ErrorCode::ERR_MALFORMED_BITSTREAM, "Failed to reconstruct Huffman tree from header");
    }

    // Calculate total valid bits in payload
    uint64_t totalPayloadBits = 0;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (header.frequencies[i] > 0) {
            totalPayloadBits += header.frequencies[i] * tree.getCode(static_cast<byte_t>(i)).length;
        }
    }

    uint64_t payloadByteCount = (totalPayloadBits + header.paddingBits) / 8;
    LimitedInputStream payloadStream(archiveStream, payloadByteCount);
    BitReader reader(payloadStream, totalPayloadBits);
    std::vector<uint8_t> outBuf;
    outBuf.reserve(DEFAULT_BUFFER_SIZE);

    if (root->isLeaf) {
        // Special Case: Single distinct symbol repeated originalSize times
        byte_t singleSym = root->symbol;
        std::vector<uint8_t> block(DEFAULT_BUFFER_SIZE, singleSym);
        filesize_t remaining = header.originalSize;

        while (remaining > 0) {
            size_t chunkSize = (remaining < DEFAULT_BUFFER_SIZE) ? static_cast<size_t>(remaining) : DEFAULT_BUFFER_SIZE;
            runningCrc = CRC32::update(runningCrc, block.data(), chunkSize);
            targetStream.write(block.data(), chunkSize);
            remaining -= chunkSize;
        }

        // Drain the single-bit payload
        for (filesize_t i = 0; i < header.originalSize; ++i) {
            if (!reader.readBit().has_value()) break;
        }
    } else {
        // Multi-symbol tree traversal (Reference Decoder)
        const HuffmanNode* curr = root;
        filesize_t decodedSymbols = 0;

        while (decodedSymbols < header.originalSize) {
            auto bitOpt = reader.readBit();
            if (!bitOpt.has_value()) {
                return Result<DecompressionStats>::fail(
                    ErrorCode::ERR_TRUNCATED_PAYLOAD,
                    "Payload bitstream ended before all declared symbols were decoded (" +
                    std::to_string(decodedSymbols) + " of " + std::to_string(header.originalSize) + ")");
            }

            curr = (*bitOpt) ? curr->right : curr->left;
            if (!curr) {
                return Result<DecompressionStats>::fail(ErrorCode::ERR_MALFORMED_BITSTREAM, "Bitstream led to null tree branch");
            }

            if (curr->isLeaf) {
                outBuf.push_back(curr->symbol);
                decodedSymbols++;
                curr = root;

                if (outBuf.size() >= DEFAULT_BUFFER_SIZE) {
                    runningCrc = CRC32::update(runningCrc, outBuf.data(), outBuf.size());
                    targetStream.write(outBuf.data(), outBuf.size());
                    outBuf.clear();
                }
            }
        }

        if (!outBuf.empty()) {
            runningCrc = CRC32::update(runningCrc, outBuf.data(), outBuf.size());
            targetStream.write(outBuf.data(), outBuf.size());
            outBuf.clear();
        }
    }

    // 6. Read and Verify Archive Trailer
    ArchiveTrailer trailer;
    auto tRes = trailer.readFromStream(archiveStream);
    if (!tRes.isOk) {
        return Result<DecompressionStats>::fail(tRes.error, tRes.message);
    }

    uint32_t finalCrc = runningCrc ^ 0xFFFFFFFF;
    if (trailer.dataCrc != finalCrc) {
        return Result<DecompressionStats>::fail(
            ErrorCode::ERR_DATA_CRC_MISMATCH,
            "Decompressed data CRC32 mismatch! Expected 0x" + std::to_string(trailer.dataCrc) +
            " but got 0x" + std::to_string(finalCrc));
    }

    targetStream.flush();
    stats.decompressedSize = targetStream.bytesWritten() - initialWritten;
    stats.dataCrc = finalCrc;

    auto endTime = std::chrono::high_resolution_clock::now();
    stats.elapsedTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    if (stats.elapsedTimeMs > 0.0) {
        double sec = stats.elapsedTimeMs / 1000.0;
        stats.throughputMBs = (static_cast<double>(stats.decompressedSize) / (1024.0 * 1024.0)) / sec;
    }

    return Result<DecompressionStats>::ok(stats);
}

} // namespace HuffmanEngine
