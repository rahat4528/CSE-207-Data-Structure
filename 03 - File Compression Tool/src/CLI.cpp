#include "CLI.h"
#include "FileIO.h"
#include "Compressor.h"
#include "Decompressor.h"
#include "PathSanitizer.h"
#include "BatchProcessor.h"
#include "BenchmarkSuite.h"
#include "TreeVisualizer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace HuffmanEngine {

void CLI::printUsage() {
    std::cout << "\n========================================================================\n"
              << "       HUFFMAN CODING ENGINE — LOSSLESS DATA COMPRESSION TOOL (C++20)   \n"
              << "========================================================================\n"
              << "Usage: huffman <command> [options]\n\n"
              << "Commands:\n"
              << "  compress <file> [-o <out>] [-f] [--stored]  Compress file to .huff\n"
              << "  decompress <archive.huff> [-o <out>] [-f]   Restore original file\n"
              << "  verify <fileA> <fileB_or_archive>           Verify byte-for-byte identity\n"
              << "  info <archive.huff>                         Display archive header metadata\n"
              << "  preview <file> [--top <N>]                  Show character frequency table\n"
              << "  stats <file>                                Dry-run entropy & redundancy report\n"
              << "  batch <dir> [-o <out_dir>] [-r] [-f]        Batch compress all files in folder\n"
              << "  benchmark                                   Run standard performance benchmark\n"
              << "  tree <file> [--format ascii|svg] [-o <out>] Visualize Huffman binary tree\n"
              << "  repl                                        Start interactive console REPL\n"
              << "========================================================================\n\n";
}

int CLI::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        runRepl();
        return 0;
    }

    const std::string& cmd = args[0];
    if (cmd == "compress") return handleCompress(args);
    if (cmd == "decompress") return handleDecompress(args);
    if (cmd == "verify") return handleVerify(args);
    if (cmd == "info") return handleInfo(args);
    if (cmd == "preview") return handlePreview(args);
    if (cmd == "stats") return handleStats(args);
    if (cmd == "batch") return handleBatch(args);
    if (cmd == "benchmark") return handleBenchmark(args);
    if (cmd == "tree") return handleTree(args);
    if (cmd == "repl") {
        runRepl();
        return 0;
    }
    if (cmd == "--help" || cmd == "-h" || cmd == "help") {
        printUsage();
        return 0;
    }

    std::cerr << "[ERROR] Unknown command: " << cmd << ". Run 'huffman --help' for usage.\n";
    return 1;
}

int CLI::handleCompress(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman compress <input_file> [-o <output_archive>] [-f] [--stored]\n";
        return 1;
    }

    std::filesystem::path inputPath = args[1];
    auto valIn = PathSanitizer::validateInputFile(inputPath);
    if (!valIn.isOk) {
        std::cerr << "[ERROR] " << valIn.message << "\n";
        return 1;
    }

    std::filesystem::path outputPath = inputPath.string() + ".huff";
    bool force = false;
    bool allowStored = false;

    for (size_t i = 2; i < args.size(); ++i) {
        if ((args[i] == "-o" || args[i] == "--output") && (i + 1 < args.size())) {
            outputPath = args[++i];
        } else if (args[i] == "-f" || args[i] == "--force") {
            force = true;
        } else if (args[i] == "--stored") {
            allowStored = true;
        }
    }

    auto valOut = PathSanitizer::validateOutputFile(inputPath, outputPath, force);
    if (!valOut.isOk) {
        std::cerr << "[ERROR] " << valOut.message << "\n";
        return 1;
    }

    FileInputStream inStream(inputPath);
    FileOutputStream outStream(outputPath, /*atomicTemp=*/true);

    std::cout << "[INFO] Compressing: " << inputPath.filename().string() << " -> " << outputPath.string() << " ...\n";
    auto compRes = Compressor::compress(inStream, outStream, inputPath.filename().string(), 0, allowStored);
    if (!compRes.isOk) {
        outStream.abort();
        std::cerr << "[ERROR] Compression failed: " << compRes.message << "\n";
        return 1;
    }

    auto commitRes = outStream.commit();
    if (!commitRes.isOk) {
        std::cerr << "[ERROR] " << commitRes.message << "\n";
        return 1;
    }

    const auto& s = compRes.value;
    std::cout << "\n================ COMPRESSION RESULTS ================\n";
    std::cout << "Original Size     : " << s.originalSize << " bytes\n";
    std::cout << "Compressed Size   : " << s.compressedSize << " bytes (Header: " << s.headerSize << ", Payload: " << s.payloadSize << ")\n";
    std::cout << "Compression Ratio : " << std::fixed << std::setprecision(4) << s.compressionRatio << " (" << std::setprecision(2) << s.spaceSavings << "% space savings)\n";
    std::cout << "Shannon Entropy   : " << std::setprecision(2) << s.entropy << " bits/symbol (Average Code Length: " << s.averageCodeLength << ")\n";
    std::cout << "Huffman Efficiency: " << std::setprecision(1) << s.efficiency << "%\n";
    std::cout << "Elapsed Time      : " << std::setprecision(2) << s.elapsedTimeMs << " ms (" << std::setprecision(1) << s.throughputMBs << " MB/s)\n";
    if (s.usedStoredFallback) {
        std::cout << "Mode Note         : Used Stored Fallback (Huffman header overhead would have increased file size)\n";
    }
    std::cout << "=====================================================\n\n";

    return 0;
}

int CLI::handleDecompress(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman decompress <archive.huff> [-o <out_path>] [-f]\n";
        return 1;
    }

    std::filesystem::path inputPath = args[1];
    auto valIn = PathSanitizer::validateInputFile(inputPath);
    if (!valIn.isOk) {
        std::cerr << "[ERROR] " << valIn.message << "\n";
        return 1;
    }

    // Inspect header first
    FileInputStream peekStream(inputPath);
    ArchiveHeader header;
    auto hRes = header.readFromStream(peekStream);
    peekStream.close();

    if (!hRes.isOk) {
        std::cerr << "[ERROR] Unable to read archive header: " << hRes.message << "\n";
        return 1;
    }

    std::filesystem::path outputPath = std::filesystem::current_path() / PathSanitizer::sanitizeFilename(header.originalFilename);
    bool force = false;

    for (size_t i = 2; i < args.size(); ++i) {
        if ((args[i] == "-o" || args[i] == "--output") && (i + 1 < args.size())) {
            std::filesystem::path userPath = args[++i];
            if (std::filesystem::is_directory(userPath)) {
                outputPath = userPath / PathSanitizer::sanitizeFilename(header.originalFilename);
            } else {
                outputPath = userPath;
            }
        } else if (args[i] == "-f" || args[i] == "--force") {
            force = true;
        }
    }

    auto valOut = PathSanitizer::validateOutputFile(inputPath, outputPath, force);
    if (!valOut.isOk) {
        std::cerr << "[ERROR] " << valOut.message << "\n";
        return 1;
    }

    FileInputStream inStream(inputPath);
    FileOutputStream outStream(outputPath, /*atomicTemp=*/true);

    std::cout << "[INFO] Decompressing: " << inputPath.filename().string() << " -> " << outputPath.string() << " ...\n";
    auto decompRes = Decompressor::decompress(inStream, outStream);
    if (!decompRes.isOk) {
        outStream.abort();
        std::cerr << "[ERROR] Decompression failed: " << decompRes.message << "\n";
        return 1;
    }

    auto commitRes = outStream.commit();
    if (!commitRes.isOk) {
        std::cerr << "[ERROR] " << commitRes.message << "\n";
        return 1;
    }

    const auto& s = decompRes.value;
    std::cout << "\n=============== DECOMPRESSION RESULTS ===============\n";
    std::cout << "Restored File     : " << outputPath.string() << "\n";
    std::cout << "Decompressed Size : " << s.decompressedSize << " bytes\n";
    std::cout << "Data CRC32        : 0x" << std::hex << std::uppercase << s.dataCrc << std::dec << " (VERIFIED MATCH)\n";
    std::cout << "Elapsed Time      : " << std::fixed << std::setprecision(2) << s.elapsedTimeMs << " ms (" << std::setprecision(1) << s.throughputMBs << " MB/s)\n";
    std::cout << "=====================================================\n\n";

    return 0;
}

int CLI::handleVerify(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: huffman verify <original_file> <restored_file_or_archive>\n";
        return 1;
    }

    std::filesystem::path pathA = args[1];
    std::filesystem::path pathB = args[2];

    auto valA = PathSanitizer::validateInputFile(pathA);
    if (!valA.isOk) { std::cerr << "[ERROR] " << valA.message << "\n"; return 1; }
    auto valB = PathSanitizer::validateInputFile(pathB);
    if (!valB.isOk) { std::cerr << "[ERROR] " << valB.message << "\n"; return 1; }

    // If file B is an archive, decompress in memory to verify
    if (pathB.extension() == ".huff") {
        std::cout << "[INFO] Verifying archive against original source...\n";
        FileInputStream arcIn(pathB);
        MemoryOutputStream decompressed;
        auto dRes = Decompressor::decompress(arcIn, decompressed);
        if (!dRes.isOk) {
            std::cerr << "[FAILED] Archive failed decompression: " << dRes.message << "\n";
            return 1;
        }

        FileInputStream srcIn(pathA);
        std::vector<uint8_t> srcBuf(decompressed.getBuffer().size());
        size_t readCount = srcIn.read(srcBuf.data(), srcBuf.size());
        if (readCount != decompressed.getBuffer().size() || srcBuf != decompressed.getBuffer()) {
            std::cerr << "[FAILED] Decompressed content does not match original file!\n";
            return 1;
        }
        std::cout << "[SUCCESS] Archive verified 100% byte-for-byte identical to original!\n";
        return 0;
    }

    // Otherwise byte-for-byte stream compare two files on disk
    FileInputStream f1(pathA);
    FileInputStream f2(pathB);

    if (f1.totalSize() != f2.totalSize()) {
        std::cerr << "[FAILED] Size mismatch: " << pathA.filename().string() << " (" << f1.totalSize()
                  << " bytes) != " << pathB.filename().string() << " (" << f2.totalSize() << " bytes)\n";
        return 1;
    }

    std::vector<uint8_t> buf1(DEFAULT_BUFFER_SIZE);
    std::vector<uint8_t> buf2(DEFAULT_BUFFER_SIZE);
    filesize_t offset = 0;

    while (!f1.isEof() && !f2.isEof()) {
        size_t n1 = f1.read(buf1.data(), DEFAULT_BUFFER_SIZE);
        size_t n2 = f2.read(buf2.data(), DEFAULT_BUFFER_SIZE);
        if (n1 != n2) {
            std::cerr << "[FAILED] Premature stream end at offset " << offset << "\n";
            return 1;
        }
        if (n1 == 0) break;

        for (size_t i = 0; i < n1; ++i) {
            if (buf1[i] != buf2[i]) {
                std::cerr << "[FAILED] First differing byte at offset " << (offset + i)
                          << ": [0x" << std::hex << static_cast<int>(buf1[i])
                          << "] != [0x" << static_cast<int>(buf2[i]) << "]\n";
                return 1;
            }
        }
        offset += n1;
    }

    std::cout << "[SUCCESS] Files are 100% byte-for-byte identical (" << offset << " bytes verified)!\n";
    return 0;
}

int CLI::handleInfo(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman info <archive.huff>\n";
        return 1;
    }

    std::filesystem::path inputPath = args[1];
    auto valIn = PathSanitizer::validateInputFile(inputPath);
    if (!valIn.isOk) { std::cerr << "[ERROR] " << valIn.message << "\n"; return 1; }

    FileInputStream inStream(inputPath);
    ArchiveHeader header;
    auto hRes = header.readFromStream(inStream);
    if (!hRes.isOk) {
        std::cerr << "[ERROR] Failed to read header: " << hRes.message << "\n";
        return 1;
    }

    std::cout << "\n================= ARCHIVE HEADER INFO =================\n";
    std::cout << "Archive File      : " << inputPath.filename().string() << " (" << std::filesystem::file_size(inputPath) << " bytes)\n";
    std::cout << "Original Filename : " << header.originalFilename << "\n";
    std::cout << "Format Version    : " << static_cast<int>(header.version) << "\n";
    std::cout << "Compression Method: " << (header.method == CompressionMethod::STORED_RAW ? "STORED (Raw Uncompressed)" : "HUFFMAN (Prefix-Free Coding)") << "\n";
    std::cout << "Original Size     : " << header.originalSize << " bytes\n";
    std::cout << "Alphabet Count    : " << header.alphabetSize << " distinct symbols\n";
    std::cout << "Padding Bits      : " << static_cast<int>(header.paddingBits) << " bits\n";
    std::cout << "Header CRC32      : 0x" << std::hex << std::uppercase << header.headerCrc << std::dec << " (VALID)\n";
    std::cout << "=======================================================\n\n";

    return 0;
}

int CLI::handlePreview(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman preview <file> [--top <N>]\n";
        return 1;
    }

    std::filesystem::path inputPath = args[1];
    auto valIn = PathSanitizer::validateInputFile(inputPath);
    if (!valIn.isOk) { std::cerr << "[ERROR] " << valIn.message << "\n"; return 1; }

    size_t topN = 25;
    for (size_t i = 2; i < args.size(); ++i) {
        if ((args[i] == "--top" || args[i] == "-n") && (i + 1 < args.size())) {
            topN = std::stoull(args[++i]);
        }
    }

    FileInputStream inStream(inputPath);
    FrequencyCounter counter;
    counter.countStream(inStream);

    HuffmanTree tree;
    tree.buildFromFrequencies(counter.getTable());
    auto sortedInfo = tree.getSortedCodeInfo(counter.getTable());

    std::cout << "\n=================== FREQUENCY & CODE PREVIEW ===================\n";
    std::cout << "File: " << inputPath.filename().string() << " (" << counter.getTotalBytes()
              << " bytes, " << counter.getDistinctSymbolCount() << " distinct symbols)\n";
    std::cout << "Shannon Entropy: " << std::fixed << std::setprecision(3) << counter.calculateEntropy() << " bits/symbol\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << std::left << std::setw(6) << "Byte"
              << std::setw(8) << "Char"
              << std::right << std::setw(12) << "Frequency"
              << std::setw(10) << "Percent"
              << std::setw(8) << "Bits"
              << "  Huffman Code\n";
    std::cout << "----------------------------------------------------------------\n";

    size_t count = 0;
    for (const auto& item : sortedInfo) {
        if (count++ >= topN) break;

        char c = static_cast<char>(item.symbol);
        std::string charDisp;
        if (c >= 32 && c <= 126) charDisp = std::string("'") + c + "'";
        else if (c == '\n') charDisp = "\\n";
        else if (c == '\r') charDisp = "\\r";
        else if (c == '\t') charDisp = "\\t";
        else charDisp = "N/A";

        double pct = (static_cast<double>(item.frequency) / static_cast<double>(counter.getTotalBytes())) * 100.0;

        std::cout << std::left << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(item.symbol) << std::dec << std::setfill(' ')
                  << "  " << std::setw(8) << charDisp
                  << std::right << std::setw(10) << item.frequency
                  << std::fixed << std::setprecision(2) << std::setw(9) << pct << "%"
                  << std::setw(6) << item.length
                  << "  " << item.code.toString() << "\n";
    }

    if (sortedInfo.size() > topN) {
        std::cout << "... (" << (sortedInfo.size() - topN) << " additional symbols omitted) ...\n";
    }
    std::cout << "================================================================\n\n";

    return 0;
}

int CLI::handleStats(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman stats <file>\n";
        return 1;
    }

    std::filesystem::path inputPath = args[1];
    auto valIn = PathSanitizer::validateInputFile(inputPath);
    if (!valIn.isOk) { std::cerr << "[ERROR] " << valIn.message << "\n"; return 1; }

    FileInputStream inStream(inputPath);
    FrequencyCounter counter;
    counter.countStream(inStream);

    HuffmanTree tree;
    tree.buildFromFrequencies(counter.getTable());

    filesize_t totalBytes = counter.getTotalBytes();
    double entropy = counter.calculateEntropy();
    uint64_t wpl = tree.calculateWeightedPathLength(counter.getTable());
    double avgCodeLen = tree.calculateAverageCodeLength(counter.getTable(), totalBytes);
    double efficiency = (avgCodeLen > 0.0) ? (entropy / avgCodeLen) * 100.0 : 0.0;

    uint64_t payloadBits = wpl;
    uint8_t padding = static_cast<uint8_t>((8 - (payloadBits % 8)) % 8);
    filesize_t payloadBytes = (payloadBits + padding) / 8;
    filesize_t headerBytes = 35 + (counter.getDistinctSymbolCount() * 9);
    filesize_t totalArchiveBytes = headerBytes + payloadBytes + 4;
    double projectedRatio = static_cast<double>(totalArchiveBytes) / static_cast<double>(totalBytes);
    double projectedSavings = (1.0 - projectedRatio) * 100.0;

    std::cout << "\n================= DRY-RUN COMPRESSION ANALYSIS =================\n";
    std::cout << "Target File          : " << inputPath.filename().string() << " (" << totalBytes << " bytes)\n";
    std::cout << "Distinct Alphabet    : " << counter.getDistinctSymbolCount() << " / 256 symbols\n";
    std::cout << "Shannon Entropy      : " << std::fixed << std::setprecision(3) << entropy << " bits/symbol\n";
    std::cout << "Theoretical Minimum  : " << counter.calculateTheoreticalMinimumBits() << " bits ("
              << (counter.calculateTheoreticalMinimumBits() + 7) / 8 << " bytes)\n";
    std::cout << "Average Code Length  : " << std::setprecision(3) << avgCodeLen << " bits/symbol\n";
    std::cout << "Huffman Efficiency   : " << std::setprecision(1) << efficiency << "%\n";
    std::cout << "Estimated Header     : " << headerBytes << " bytes\n";
    std::cout << "Estimated Payload    : " << payloadBytes << " bytes (" << static_cast<int>(padding) << " padding bits)\n";
    std::cout << "Projected Total Size : " << totalArchiveBytes << " bytes\n";
    std::cout << "Projected Savings    : " << std::setprecision(2) << projectedSavings << "%\n";
    std::cout << "================================================================\n\n";

    return 0;
}

int CLI::handleBatch(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman batch <directory> [-o <out_dir>] [-r] [-f]\n";
        return 1;
    }

    std::filesystem::path dir = args[1];
    std::filesystem::path outDir = dir / "huffman_archives";
    bool recursive = false;
    bool force = false;

    for (size_t i = 2; i < args.size(); ++i) {
        if ((args[i] == "-o" || args[i] == "--output") && (i + 1 < args.size())) {
            outDir = args[++i];
        } else if (args[i] == "-r" || args[i] == "--recursive") {
            recursive = true;
        } else if (args[i] == "-f" || args[i] == "--force") {
            force = true;
        }
    }

    std::cout << "[INFO] Running batch compression on folder: " << dir.string() << " ...\n";
    auto summary = BatchProcessor::compressDirectory(dir, outDir, recursive, force);

    std::cout << "\n================= BATCH SUMMARY =================\n";
    std::cout << "Files Processed   : " << summary.totalFiles << "\n";
    std::cout << "Successful        : " << summary.successfulFiles << "\n";
    std::cout << "Failed            : " << summary.failedFiles << "\n";
    std::cout << "Original Bytes    : " << summary.totalOriginalBytes << " bytes\n";
    std::cout << "Compressed Bytes  : " << summary.totalCompressedBytes << " bytes\n";
    std::cout << "Overall Savings   : " << std::fixed << std::setprecision(2) << summary.overallSavings << "%\n";
    std::cout << "Total Elapsed Time: " << summary.totalTimeMs << " ms\n";
    std::cout << "=================================================\n\n";

    return (summary.failedFiles == 0) ? 0 : 1;
}

int CLI::handleBenchmark(const std::vector<std::string>&) {
    std::cout << "[INFO] Executing comprehensive throughput and compression benchmark suite...\n";
    auto results = BenchmarkSuite::runAllBenchmarks(3);
    BenchmarkSuite::printBenchmarkReport(results, std::cout);
    return 0;
}

int CLI::handleTree(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: huffman tree <file> [--format ascii|svg] [-o <out.svg>]\n";
        return 1;
    }

    std::filesystem::path inputPath = args[1];
    auto valIn = PathSanitizer::validateInputFile(inputPath);
    if (!valIn.isOk) { std::cerr << "[ERROR] " << valIn.message << "\n"; return 1; }

    std::string format = "ascii";
    std::string svgOut = "huffman_tree.svg";

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--format" && (i + 1 < args.size())) {
            format = args[++i];
        } else if ((args[i] == "-o" || args[i] == "--output") && (i + 1 < args.size())) {
            svgOut = args[++i];
        }
    }

    FileInputStream inStream(inputPath);
    FrequencyCounter counter;
    counter.countStream(inStream);

    HuffmanTree tree;
    tree.buildFromFrequencies(counter.getTable());

    if (format == "svg") {
        auto res = TreeVisualizer::exportSvg(tree, svgOut);
        if (res.isOk) {
            std::cout << "[SUCCESS] Exported vector SVG tree diagram to: " << svgOut << "\n";
            return 0;
        } else {
            std::cerr << "[ERROR] " << res.message << "\n";
            return 1;
        }
    }

    TreeVisualizer::printAsciiTree(tree, std::cout);
    return 0;
}

void CLI::runRepl() {
    std::cout << "\n=======================================================\n";
    std::cout << "  HUFFMAN CODING ENGINE — INTERACTIVE REPL CONSOLE     \n";
    std::cout << "  Type 'help' for command list, or 'exit' to quit.    \n";
    std::cout << "=======================================================\n\n";

    std::string line;
    while (true) {
        std::cout << "huffman> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::vector<std::string> args;
        std::string token;
        while (iss >> token) {
            args.push_back(token);
        }

        if (args.empty()) continue;
        if (args[0] == "exit" || args[0] == "quit") {
            std::cout << "Exiting Huffman Engine REPL.\n";
            break;
        }

        execute(args);
    }
}

} // namespace HuffmanEngine
