#include "CLI.h"
#include "Resolver.h"
#include "Exporter.h"
#include "Visualizer.h"
#include "Persistence.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace DepResolver {

CLI::CLI() {
    loadPersistentState();
    if (registry_.packageCount() == 0) {
        seedDefaultDemoData();
    }
}

void CLI::seedDefaultDemoData() {
    // Seed standard packages for demonstration
    registry_.registerPackage("react", "Declarative UI Framework");
    registry_.registerVersion("react", PackageVersion(SemVer::parse("18.2.0").value()));
    registry_.registerVersion("react", PackageVersion(SemVer::parse("17.0.2").value()));

    registry_.registerPackage("nextjs", "The React Framework for the Web");
    registry_.registerVersion("nextjs", PackageVersion(SemVer::parse("14.0.0").value(), {
        DependencyDecl("react", Constraint::parse("^18.0.0").value())
    }));

    registry_.registerPackage("express", "Fast, unopinionated, minimalist web framework");
    registry_.registerVersion("express", PackageVersion(SemVer::parse("4.18.2").value(), {
        DependencyDecl("cookie-parser", Constraint::parse("^1.4.0").value())
    }));

    registry_.registerPackage("cookie-parser", "Parse Cookie header and populate req.cookies");
    registry_.registerVersion("cookie-parser", PackageVersion(SemVer::parse("1.4.6").value()));

    registry_.registerPackage("database-driver", "SQL Driver Interface");
    registry_.registerVersion("database-driver", PackageVersion(SemVer::parse("2.1.0").value()));
    registry_.registerVersion("database-driver", PackageVersion(SemVer::parse("1.0.0").value()));

    // Cyclic example package
    registry_.registerPackage("cyclic-a", "Cycle component A");
    registry_.registerVersion("cyclic-a", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("cyclic-b", Constraint::any())
    }));
    registry_.registerPackage("cyclic-b", "Cycle component B");
    registry_.registerVersion("cyclic-b", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("cyclic-a", Constraint::any())
    }));
}

void CLI::loadPersistentState() {
    auto regRes = Persistence::loadRegistry(registryFile_);
    if (regRes.isOk()) registry_ = std::move(regRes.value());

    auto stateRes = Persistence::loadInstalledState(stateFile_);
    if (stateRes.isOk()) installedState_ = std::move(stateRes.value());

    Persistence::loadHistory(history_, historyFile_);

    // Check for crash recovery
    auto recRes = Persistence::performCrashRecovery(journalFile_, installedState_);
    if (recRes.isOk() && recRes.value()) {
        std::cout << "[NOTICE] Uncommitted transaction detected in journal. Crash recovery successfully rolled back inconsistent state.\n";
    }
}

void CLI::savePersistentState() {
    Persistence::saveRegistry(registry_, registryFile_);
    Persistence::saveInstalledState(installedState_, stateFile_);
    Persistence::saveHistory(history_, historyFile_);
}

void CLI::cmdHelp() {
    std::cout << "\n=======================================================\n";
    std::cout << "  PROJECT 05: MINI PACKAGE MANAGER & DEPENDENCY RESOLVER\n";
    std::cout << "=======================================================\n";
    std::cout << "Usage: pkg-mgr [command] [arguments...]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  resolve <pkgs...>       Resolve dependencies and generate topological installation plan\n";
    std::cout << "  install <pkgs...>       Simulate installation with atomic transaction & rollback\n";
    std::cout << "  install --dry-run <...> Plan installation without applying changes\n";
    std::cout << "  tree <pkg>              Render ASCII dependency tree with cycle/diamond detection\n";
    std::cout << "  why <pkg>               Show reverse dependency path (why is package needed)\n";
    std::cout << "  svg [file.svg]          Generate standalone vector SVG visualization of graph\n";
    std::cout << "  orphans                 Detect unneeded packages installed only as dependencies\n";
    std::cout << "  autoremove              Safely remove orphaned packages\n";
    std::cout << "  history                 Display transaction history and simulation logs\n";
    std::cout << "  doctor                  Check registry, installed state, and journal consistency\n";
    std::cout << "  repl                    Launch interactive console menu\n";
    std::cout << "  help                    Display this help guide\n\n";
}

void CLI::cmdResolve(const std::vector<std::string>& pkgs, bool newestFirst) {
    if (pkgs.empty()) {
        std::cout << "[ERROR] Please specify at least one package name to resolve.\n";
        return;
    }

    ResolutionRequest req;
    req.newestFirst = newestFirst;
    for (const auto& p : pkgs) {
        req.rootRequirements.emplace_back(p, Constraint::any());
    }

    std::cout << "\n[RESOLVER] Running backtracking constraint solver for: ";
    for (const auto& p : pkgs) std::cout << p << " ";
    std::cout << "\n";

    auto res = Resolver::resolve(registry_, req);

    if (res.isSuccess()) {
        std::cout << "\033[32m[SUCCESS]\033[0m Resolution completed in " << res.stepsTaken << " steps.\n";
        std::cout << "\nResolved Packages (" << res.selectedVersions.size() << "):\n";
        for (const auto& [name, ver] : res.selectedVersions) {
            std::cout << "  - " << name << " @" << ver.toString() << "\n";
        }

        std::cout << "\nTopological Installation Plan (" << res.plan.steps.size() << " steps across " 
                  << res.plan.totalLevels << " parallel levels):\n";
        for (const auto& step : res.plan.steps) {
            std::cout << "  Step " << step.order + 1 << ": " << step.packageName << " @" << step.version 
                      << " (Level " << step.level << ") [" << step.reason << "]\n";
        }

        // Export Lockfile
        std::string lockContent = Exporter::generateLockfile(res.selectedVersions, res.plan, registry_);
        std::ofstream lockFile("pkg.lock");
        if (lockFile.is_open()) {
            lockFile << lockContent;
            std::cout << "\n[LOCKFILE] Successfully written to 'pkg.lock'.\n";
        }
    } else {
        std::cout << "\033[31m[FAILED]\033[0m Status: " << resolutionStatusToString(res.status) << "\n";
        for (const auto& diag : res.diagnostics) {
            std::cout << "  [" << errorCodeToString(diag.code) << "] " << diag.message << "\n";
            if (!diag.dependencyChain.empty()) {
                std::cout << "    Dependency Chain: " << diag.chainToString() << "\n";
            }
        }
    }
}

void CLI::cmdInstall(const std::vector<std::string>& pkgs, bool dryRun) {
    if (pkgs.empty()) {
        std::cout << "[ERROR] Please specify packages to install.\n";
        return;
    }

    ResolutionRequest req;
    for (const auto& p : pkgs) req.rootRequirements.emplace_back(p, Constraint::any());

    auto res = Resolver::resolve(registry_, req);
    if (!res.isSuccess()) {
        std::cout << "\033[31m[ERROR]\033[0m Cannot install: resolution failed.\n";
        for (const auto& diag : res.diagnostics) std::cout << "  - " << diag.message << "\n";
        return;
    }

    if (dryRun) {
        std::cout << "\n[DRY RUN] Installation plan (no changes applied):\n";
        for (const auto& step : res.plan.steps) {
            std::cout << "  * Would install: " << step.packageName << " @" << step.version << "\n";
        }
        return;
    }

    std::cout << "\n[SIMULATOR] Beginning atomic installation transaction...\n";
    auto txRes = txManager_.executePlan(installedState_, res.plan, res.selectedVersions, simEngine_);

    for (const auto& log : txRes.logs) {
        std::cout << "  " << log.toString() << "\n";
    }

    if (txRes.isSuccess()) {
        std::cout << "\033[32m[COMMITTED]\033[0m " << txRes.summary << "\n";
    } else {
        std::cout << "\033[31m[ROLLED BACK]\033[0m " << txRes.summary << "\n";
    }

    history_.append({txRes.transactionId, "2026-11-07T13:30:00", "install", txRes.status, 
                     txRes.successfullyInstalled, txRes.summary, txRes.logs});
    savePersistentState();
}

void CLI::cmdTree(const std::string& pkg) {
    ResolutionRequest req;
    req.rootRequirements.emplace_back(pkg, Constraint::any());
    auto res = Resolver::resolve(registry_, req);
    if (!res.isSuccess()) {
        std::cout << "[ERROR] Could not resolve graph for '" << pkg << "'\n";
        return;
    }
    std::cout << "\n" << Visualizer::renderAsciiTree(res.resolvedGraph, pkg) << "\n";
}

void CLI::cmdWhy(const std::string& pkg) {
    // Collect all installed dependencies graph
    DependencyGraph graph;
    for (const auto& [name, ipkg] : installedState_.getAll()) {
        graph.getOrAddNode(name);
        const auto* p = registry_.getPackage(name);
        if (!p) continue;
        const auto* pv = p->findVersion(ipkg.version);
        if (!pv) continue;
        for (const auto& d : pv->dependencies) {
            if (installedState_.isInstalled(d.packageName)) {
                graph.addEdge(name, d.packageName);
            }
        }
    }
    std::cout << "\n" << Visualizer::renderWhyTree(graph, pkg) << "\n";
}

void CLI::cmdSvg(const std::string& outputFile) {
    DependencyGraph graph;
    for (const auto& [name, ipkg] : installedState_.getAll()) {
        graph.getOrAddNode(name);
        const auto* p = registry_.getPackage(name);
        if (!p) continue;
        const auto* pv = p->findVersion(ipkg.version);
        if (!pv) continue;
        for (const auto& d : pv->dependencies) {
            if (installedState_.isInstalled(d.packageName)) {
                graph.addEdge(name, d.packageName);
            }
        }
    }

    std::string svg = Visualizer::renderSvg(graph, "Installed Packages Dependency Graph");
    std::ofstream out(outputFile);
    if (out.is_open()) {
        out << svg;
        std::cout << "[SUCCESS] Standalone SVG graph generated at: " << outputFile << "\n";
    } else {
        std::cout << "[ERROR] Failed to write SVG to: " << outputFile << "\n";
    }
}

void CLI::cmdOrphans() {
    auto orphans = StateManager::findOrphans(installedState_, registry_);
    std::cout << "\n=== Orphaned Packages Analysis ===\n";
    if (orphans.empty()) {
        std::cout << "No orphaned packages found. Installed state is fully clean.\n";
    } else {
        std::cout << "Found " << orphans.size() << " orphaned packages:\n";
        for (const auto& o : orphans) std::cout << "  - " << o << "\n";
        std::cout << "\nTip: Run 'pkg-mgr autoremove' to uninstall them safely.\n";
    }
}

void CLI::cmdAutoRemove() {
    auto removed = StateManager::autoRemoveOrphans(installedState_, registry_);
    if (removed.empty()) {
        std::cout << "No orphans to remove.\n";
    } else {
        std::cout << "\033[32m[REMOVED]\033[0m Successfully uninstalled " << removed.size() << " orphans:\n";
        for (const auto& r : removed) std::cout << "  - " << r << "\n";
        savePersistentState();
    }
}

void CLI::cmdHistory() {
    auto records = history_.getAll();
    std::cout << "\n=== Package Manager Transaction History ===\n";
    if (records.empty()) {
        std::cout << "No transaction history recorded yet.\n";
    } else {
        for (const auto& rec : records) {
            std::cout << "Tx #" << rec.transactionId << " [" << transactionStatusToString(rec.status) << "] "
                      << rec.actionName << " -> " << rec.summary << "\n";
        }
    }
}

void CLI::cmdDoctor() {
    std::cout << "\n=== Package Manager Health & Consistency Doctor ===\n";
    std::cout << "[✓] Registry: " << registry_.packageCount() << " registered packages.\n";
    std::cout << "[✓] Installed State: " << installedState_.count() << " installed packages.\n";
    std::cout << "[✓] History Journal: " << history_.size() << " logged transactions.\n";

    auto orphans = StateManager::findOrphans(installedState_, registry_);
    if (!orphans.empty()) {
        std::cout << "[!] Notice: " << orphans.size() << " orphaned package(s) detected.\n";
    } else {
        std::cout << "[✓] Zero orphaned packages detected.\n";
    }
    std::cout << "[✓] System state is healthy and consistent.\n";
}

void CLI::runInteractiveRepl() {
    std::cout << "\n=======================================================\n";
    std::cout << "   DEPENDENCY RESOLVER & PACKAGE MANAGER CONSOLE REPL\n";
    std::cout << "=======================================================\n";
    std::cout << " Type 'help' for commands, or 'exit' to quit.\n\n";

    std::string line;
    while (true) {
        std::cout << "pkg-mgr> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "exit" || line == "quit") break;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        std::vector<std::string> args;
        std::string arg;
        while (ss >> arg) args.push_back(arg);

        if (cmd == "help") cmdHelp();
        else if (cmd == "resolve") cmdResolve(args);
        else if (cmd == "install") {
            bool dry = false;
            std::vector<std::string> pkgs;
            for (const auto& a : args) {
                if (a == "--dry-run") dry = true;
                else pkgs.push_back(a);
            }
            cmdInstall(pkgs, dry);
        }
        else if (cmd == "tree") {
            if (args.empty()) std::cout << "Usage: tree <pkg>\n";
            else cmdTree(args[0]);
        }
        else if (cmd == "why") {
            if (args.empty()) std::cout << "Usage: why <pkg>\n";
            else cmdWhy(args[0]);
        }
        else if (cmd == "svg") {
            std::string out = args.empty() ? "dependency_graph.svg" : args[0];
            cmdSvg(out);
        }
        else if (cmd == "orphans") cmdOrphans();
        else if (cmd == "autoremove") cmdAutoRemove();
        else if (cmd == "history") cmdHistory();
        else if (cmd == "doctor") cmdDoctor();
        else {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for instructions.\n";
        }
    }
}

int CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        runInteractiveRepl();
        return 0;
    }

    std::string cmd = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) args.push_back(argv[i]);

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        cmdHelp();
    } else if (cmd == "repl") {
        runInteractiveRepl();
    } else if (cmd == "resolve") {
        cmdResolve(args);
    } else if (cmd == "install") {
        bool dry = false;
        std::vector<std::string> pkgs;
        for (const auto& a : args) {
            if (a == "--dry-run") dry = true;
            else pkgs.push_back(a);
        }
        cmdInstall(pkgs, dry);
    } else if (cmd == "tree") {
        if (args.empty()) { std::cout << "Usage: pkg-mgr tree <pkg>\n"; return 1; }
        cmdTree(args[0]);
    } else if (cmd == "why") {
        if (args.empty()) { std::cout << "Usage: pkg-mgr why <pkg>\n"; return 1; }
        cmdWhy(args[0]);
    } else if (cmd == "svg") {
        std::string out = args.empty() ? "dependency_graph.svg" : args[0];
        cmdSvg(out);
    } else if (cmd == "orphans") {
        cmdOrphans();
    } else if (cmd == "autoremove") {
        cmdAutoRemove();
    } else if (cmd == "history") {
        cmdHistory();
    } else if (cmd == "doctor") {
        cmdDoctor();
    } else {
        std::cout << "Unknown command: " << cmd << ". Run 'pkg-mgr help' for options.\n";
        return 1;
    }

    return 0;
}

} // namespace DepResolver

