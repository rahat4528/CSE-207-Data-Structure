#pragma once

#include "Common.h"
#include "Registry.h"
#include "StateManager.h"
#include "HistoryManager.h"
#include "SimulationEngine.h"
#include "TransactionManager.h"
#include <string>
#include <vector>

namespace DepResolver {

class CLI {
private:
    Registry registry_;
    InstalledState installedState_;
    HistoryManager history_;
    SimulationEngine simEngine_;
    TransactionManager txManager_;

    std::string registryFile_{"data_registry.manifest"};
    std::string stateFile_{"data_installed.state"};
    std::string historyFile_{"data_history.log"};
    std::string journalFile_{"data_tx.journal"};

public:
    CLI();

    void seedDefaultDemoData();
    void loadPersistentState();
    void savePersistentState();

    int run(int argc, char* argv[]);
    void runInteractiveRepl();

    // Command implementations
    void cmdHelp();
    void cmdResolve(const std::vector<std::string>& pkgs, bool newestFirst = true);
    void cmdInstall(const std::vector<std::string>& pkgs, bool dryRun = false);
    void cmdTree(const std::string& pkg);
    void cmdWhy(const std::string& pkg);
    void cmdSvg(const std::string& outputFile = "dependency_graph.svg");
    void cmdOrphans();
    void cmdAutoRemove();
    void cmdHistory();
    void cmdDoctor();
};

} // namespace DepResolver

