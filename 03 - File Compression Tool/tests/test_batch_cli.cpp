#include "TestRunner.h"
#include "TreeVisualizer.h"
#include "BenchmarkSuite.h"
#include "CLI.h"
#include "FrequencyCounter.h"
#include "HuffmanTree.h"
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace HuffmanEngine;

TEST_CASE(test_tree_visualizer_ascii_and_svg) {
    FrequencyCounter counter;
    std::string text = "HUFFMAN_TREE_VISUALIZATION_TEST_ABRACADABRA";
    MemoryInputStream stream(text);
    counter.countStream(stream);

    HuffmanTree tree;
    tree.buildFromFrequencies(counter.getTable());

    // 1. ASCII tree stream output
    std::ostringstream ss;
    TreeVisualizer::printAsciiTree(tree, ss, 4);
    std::string asciiOut = ss.str();
    ASSERT_TRUE(asciiOut.find("=== HUFFMAN TREE VISUALIZATION ===") != std::string::npos);
    ASSERT_TRUE(asciiOut.find("LEAF:") != std::string::npos);

    // 2. SVG export
    std::filesystem::path svgPath = "test_out/test_tree.svg";
    std::filesystem::create_directories("test_out");
    std::filesystem::remove(svgPath);

    auto svgRes = TreeVisualizer::exportSvg(tree, svgPath.string(), 5);
    ASSERT_TRUE(svgRes.isOk);
    ASSERT_TRUE(std::filesystem::exists(svgPath));

    {
        std::ifstream svgFile(svgPath);
        std::string svgContent((std::istreambuf_iterator<char>(svgFile)), std::istreambuf_iterator<char>());
        ASSERT_TRUE(svgContent.find("<svg") != std::string::npos);
        ASSERT_TRUE(svgContent.find("</svg>") != std::string::npos);
        ASSERT_TRUE(svgContent.find("<circle") != std::string::npos);
    }

    std::filesystem::remove(svgPath);
}

TEST_CASE(test_benchmark_suite_execution) {
    auto results = BenchmarkSuite::runAllBenchmarks(1);
    ASSERT_EQ(results.size(), 5);

    for (const auto& r : results) {
        ASSERT_GT(r.originalSize, 0);
        ASSERT_GT(r.compressedSize, 0);
        ASSERT_GE(r.compressThroughputMBs, 1.0); // At least 1 MB/s
        ASSERT_GE(r.decompressThroughputMBs, 1.0);
    }
}

TEST_CASE(test_cli_subcommands_execution) {
    std::filesystem::create_directories("test_out");
    std::filesystem::path testFile = "test_out/cli_test.txt";
    std::filesystem::path arcFile = "test_out/cli_test.txt.huff";
    std::filesystem::path restFile = "test_out/cli_restored.txt";

    {
        std::ofstream f(testFile);
        f << "Testing Huffman CLI subcommands: compress, info, preview, stats, and decompress.";
    }

    // 1. Help command
    ASSERT_EQ(CLI::execute({"--help"}), 0);

    // 2. Stats dry-run
    ASSERT_EQ(CLI::execute({"stats", testFile.string()}), 0);

    // 3. Preview frequency table
    ASSERT_EQ(CLI::execute({"preview", testFile.string(), "--top", "5"}), 0);

    // 4. Compress
    ASSERT_EQ(CLI::execute({"compress", testFile.string(), "-o", arcFile.string(), "-f"}), 0);
    ASSERT_TRUE(std::filesystem::exists(arcFile));

    // 5. Info
    ASSERT_EQ(CLI::execute({"info", arcFile.string()}), 0);

    // 6. Decompress
    ASSERT_EQ(CLI::execute({"decompress", arcFile.string(), "-o", restFile.string(), "-f"}), 0);
    ASSERT_TRUE(std::filesystem::exists(restFile));

    // 7. Verify
    ASSERT_EQ(CLI::execute({"verify", testFile.string(), restFile.string()}), 0);

    std::filesystem::remove(testFile);
    std::filesystem::remove(arcFile);
    std::filesystem::remove(restFile);
}
