#include "TimeTravelStore.h"
#include "CLI.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string walPath = "data/store.wal";
    bool runDemo = false;
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--wal" && i + 1 < argc) {
            walPath = argv[++i];
        } else if (arg == "--demo") {
            runDemo = true;
        } else if (arg == "--memory") {
            walPath = ""; // Pure in-memory mode
        } else {
            args.push_back(arg);
        }
    }

    ttkv::TimeTravelStore store(walPath);
    auto initRes = store.initialize(true); // Recover with auto-truncation of torn tails
    if (initRes.isError()) {
        std::cerr << "Initialization warning: " << initRes.message() << std::endl;
    }

    ttkv::CLI cli(store);

    if (runDemo) {
        std::cout << cli.runDemo() << std::endl;
        return 0;
    }

    if (!args.empty()) {
        std::string fullCmd;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) fullCmd += " ";
            fullCmd += args[i];
        }
        std::string output = cli.executeCommand(fullCmd);
        std::cout << output << std::endl;
        return 0;
    }

    // Interactive REPL
    cli.runInteractive();
    return 0;
}
