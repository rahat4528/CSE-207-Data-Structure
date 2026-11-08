#pragma once

#include "Common.h"
#include "TimeTravelStore.h"
#include <string>
#include <vector>
#include <iostream>

namespace ttkv {

class CLI {
public:
    explicit CLI(TimeTravelStore& store);

    // Runs single command string and returns output
    std::string executeCommand(const std::string& line);

    // Interactive REPL loop
    void runInteractive(std::istream& in = std::cin, std::ostream& out = std::cout);

    // Demo / seed walkthrough showcasing all features
    std::string runDemo();

    // Splits command line into tokens respecting double and single quotes
    static std::vector<std::string> tokenize(const std::string& line);

private:
    TimeTravelStore& m_store;
    std::string handleGet(const std::vector<std::string>& tokens);
    std::string handleSet(const std::vector<std::string>& tokens);
    std::string handleDelete(const std::vector<std::string>& tokens);
    std::string handleHistory(const std::vector<std::string>& tokens);
    std::string handleDiff(const std::vector<std::string>& tokens);
    std::string handleSnapshot(const std::vector<std::string>& tokens);
    std::string handleRollback(const std::vector<std::string>& tokens);
    std::string handleSearch(const std::vector<std::string>& tokens);
    std::string handleStats();
    std::string handleHelp();
};

} // namespace ttkv
