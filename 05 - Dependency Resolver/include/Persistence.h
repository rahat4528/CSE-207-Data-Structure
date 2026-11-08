#pragma once

#include "Common.h"
#include "Registry.h"
#include "StateManager.h"
#include "HistoryManager.h"
#include <string>
#include <filesystem>

namespace DepResolver {

class Persistence {
public:
    // Atomic save via .tmp file and atomic rename
    static Result<void> saveRegistry(const Registry& registry, const std::string& filePath);
    static Result<Registry> loadRegistry(const std::string& filePath);

    static Result<void> saveInstalledState(const InstalledState& state, const std::string& filePath);
    static Result<InstalledState> loadInstalledState(const std::string& filePath);

    static Result<void> saveHistory(const HistoryManager& history, const std::string& filePath);
    static Result<void> loadHistory(HistoryManager& history, const std::string& filePath);

    // Journal Operations
    static Result<void> appendJournal(const std::string& journalPath, const std::string& line);
    static Result<void> clearJournal(const std::string& journalPath);

    // Startup Crash Recovery: Checks for uncommitted transactions in journal
    // Returns true if recovery rollback was performed, false if journal was clean
    static Result<bool> performCrashRecovery(const std::string& journalPath, InstalledState& state);

    // Compute simple robust 64-bit checksum for integrity verification
    static uint64_t computeChecksum(std::string_view text);
};

} // namespace DepResolver
