#ifndef HUFFMAN_BENCHMARK_SUITE_H
#define HUFFMAN_BENCHMARK_SUITE_H

#include "Common.h"
#include <string>
#include <vector>

namespace HuffmanEngine {

struct BenchmarkResult {
    std::string profileName;
    filesize_t originalSize{0};
    filesize_t compressedSize{0};
    double compressionRatio{0.0};
    double spaceSavings{0.0};
    double entropy{0.0};
    double compressThroughputMBs{0.0};
    double decompressThroughputMBs{0.0};
    double compressTimeMs{0.0};
    double decompressTimeMs{0.0};
};

class BenchmarkSuite {
public:
    static std::vector<BenchmarkResult> runAllBenchmarks(int iterations = 3);
    static void printBenchmarkReport(const std::vector<BenchmarkResult>& results, std::ostream& os);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_BENCHMARK_SUITE_H
