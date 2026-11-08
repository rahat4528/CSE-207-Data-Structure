#include "BenchmarkSuite.h"
#include "Compressor.h"
#include "Decompressor.h"
#include <iostream>
#include <iomanip>
#include <random>

namespace HuffmanEngine {

namespace {

std::vector<uint8_t> generateEnglishText(size_t targetSize) {
    const std::string corpus =
        "The Huffman coding algorithm is a cornerstone of modern digital communication and data compression. "
        "Invented by David A. Huffman while he was a Ph.D. student at MIT in 1952, it represents an optimal prefix code. "
        "Frequent characters receive short codewords, while rare characters receive longer codewords. ";
    std::vector<uint8_t> data;
    data.reserve(targetSize);
    while (data.size() < targetSize) {
        data.insert(data.end(), corpus.begin(), corpus.end());
    }
    data.resize(targetSize);
    return data;
}

std::vector<uint8_t> generateCppSource(size_t targetSize) {
    const std::string corpus =
        "#include <iostream>\n#include <vector>\n#include <string>\n"
        "template <typename T> class MinHeap {\npublic:\n    void push(T val) { data.push_back(val); }\n"
        "    T pop() { T top = data.front(); data.pop_back(); return top; }\nprivate:\n    std::vector<T> data;\n};\n";
    std::vector<uint8_t> data;
    data.reserve(targetSize);
    while (data.size() < targetSize) {
        data.insert(data.end(), corpus.begin(), corpus.end());
    }
    data.resize(targetSize);
    return data;
}

std::vector<uint8_t> generateUnicodeCorpus(size_t targetSize) {
    const std::string corpus = "বাংলা ভাষা ও সাহিত্য। Hello World! 你好世界！ Bonjour le monde! 🚀🔥💻 ";
    std::vector<uint8_t> data;
    data.reserve(targetSize);
    while (data.size() < targetSize) {
        data.insert(data.end(), corpus.begin(), corpus.end());
    }
    data.resize(targetSize);
    return data;
}

std::vector<uint8_t> generateSkewedData(size_t targetSize) {
    std::vector<uint8_t> data;
    data.reserve(targetSize);
    // 70% 'A', 20% 'B', 8% 'C', 2% 'D'
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 100);
    for (size_t i = 0; i < targetSize; ++i) {
        int r = dist(rng);
        if (r <= 70) data.push_back('A');
        else if (r <= 90) data.push_back('B');
        else if (r <= 98) data.push_back('C');
        else data.push_back('D');
    }
    return data;
}

std::vector<uint8_t> generateRandomBinary(size_t targetSize) {
    std::vector<uint8_t> data(targetSize);
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < targetSize; ++i) {
        data[i] = static_cast<uint8_t>(dist(rng));
    }
    return data;
}

} // anonymous namespace

std::vector<BenchmarkResult> BenchmarkSuite::runAllBenchmarks(int iterations) {
    struct BenchmarkTarget {
        std::string name;
        std::vector<uint8_t> data;
    };

    constexpr size_t PROFILE_SIZE = 100 * 1024; // 100 KB per test profile

    std::vector<BenchmarkTarget> targets = {
        {"Plain English Text", generateEnglishText(PROFILE_SIZE)},
        {"C++ Source Code", generateCppSource(PROFILE_SIZE)},
        {"Multilingual Unicode", generateUnicodeCorpus(PROFILE_SIZE)},
        {"Highly Skewed Data", generateSkewedData(PROFILE_SIZE)},
        {"Random Binary Data", generateRandomBinary(PROFILE_SIZE)}
    };

    std::vector<BenchmarkResult> results;

    for (const auto& target : targets) {
        BenchmarkResult res;
        res.profileName = target.name;
        res.originalSize = target.data.size();

        double bestCompTime = 1e9;
        double bestDecompTime = 1e9;
        CompressionStats lastStats;

        for (int iter = 0; iter < iterations; ++iter) {
            MemoryInputStream inStream(target.data);
            MemoryOutputStream arcOut;

            auto compRes = Compressor::compress(inStream, arcOut, "bench.bin", 0, /*allowStoredFallback=*/false);
            if (!compRes.isOk) continue;
            lastStats = compRes.value;

            if (lastStats.elapsedTimeMs < bestCompTime) {
                bestCompTime = lastStats.elapsedTimeMs;
            }

            MemoryInputStream arcIn(arcOut.getBuffer());
            MemoryOutputStream restoredOut;
            auto decompRes = Decompressor::decompress(arcIn, restoredOut);
            if (!decompRes.isOk) continue;

            if (decompRes.value.elapsedTimeMs < bestDecompTime) {
                bestDecompTime = decompRes.value.elapsedTimeMs;
            }
        }

        res.compressedSize = lastStats.compressedSize;
        res.compressionRatio = lastStats.compressionRatio;
        res.spaceSavings = lastStats.spaceSavings;
        res.entropy = lastStats.entropy;
        res.compressTimeMs = bestCompTime;
        res.decompressTimeMs = bestDecompTime;

        double sizeMB = static_cast<double>(res.originalSize) / (1024.0 * 1024.0);
        if (bestCompTime > 0.0) {
            res.compressThroughputMBs = sizeMB / (bestCompTime / 1000.0);
        }
        if (bestDecompTime > 0.0) {
            res.decompressThroughputMBs = sizeMB / (bestDecompTime / 1000.0);
        }

        results.push_back(res);
    }

    return results;
}

void BenchmarkSuite::printBenchmarkReport(const std::vector<BenchmarkResult>& results, std::ostream& os) {
    os << "\n========================================================================================================\n";
    os << "                           HUFFMAN ENGINE COMPRESSION & THROUGHPUT BENCHMARK                           \n";
    os << "========================================================================================================\n";
    os << std::left << std::setw(24) << "Profile"
       << std::right << std::setw(12) << "Orig Size"
       << std::setw(12) << "Comp Size"
       << std::setw(10) << "Savings"
       << std::setw(10) << "Entropy"
       << std::setw(15) << "Comp (MB/s)"
       << std::setw(16) << "Decomp (MB/s)"
       << "\n";
    os << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        os << std::left << std::setw(24) << r.profileName
           << std::right << std::setw(10) << r.originalSize << " B"
           << std::setw(10) << r.compressedSize << " B"
           << std::fixed << std::setprecision(1) << std::setw(9) << r.spaceSavings << "%"
           << std::setprecision(2) << std::setw(10) << r.entropy
           << std::setprecision(1) << std::setw(15) << r.compressThroughputMBs
           << std::setw(16) << r.decompressThroughputMBs
           << "\n";
    }
    os << "========================================================================================================\n\n";
}

} // namespace HuffmanEngine
