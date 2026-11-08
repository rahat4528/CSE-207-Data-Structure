#ifndef HUFFMAN_CLI_H
#define HUFFMAN_CLI_H

#include "Common.h"
#include <string>
#include <vector>

namespace HuffmanEngine {

class CLI {
public:
    static int execute(const std::vector<std::string>& args);
    static void runRepl();
    static void printUsage();

private:
    static int handleCompress(const std::vector<std::string>& args);
    static int handleDecompress(const std::vector<std::string>& args);
    static int handleVerify(const std::vector<std::string>& args);
    static int handleInfo(const std::vector<std::string>& args);
    static int handlePreview(const std::vector<std::string>& args);
    static int handleStats(const std::vector<std::string>& args);
    static int handleBatch(const std::vector<std::string>& args);
    static int handleBenchmark(const std::vector<std::string>& args);
    static int handleTree(const std::vector<std::string>& args);
};

} // namespace HuffmanEngine

#endif // HUFFMAN_CLI_H
