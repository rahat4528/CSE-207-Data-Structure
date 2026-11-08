#pragma once

#include "Common.h"
#include "Resolver.h"
#include "StateManager.h"
#include "HistoryManager.h"
#include <string>
#include <vector>
#include <map>

namespace DepResolver {

// ============================================================================
// Configuration Comparison Diff (A10)
// ============================================================================
struct ConfigDiff {
    std::vector<std::string> addedPackages;
    std::vector<std::string> removedPackages;
    std::vector<VersionTransition> changedPackages;

    bool hasDifferences() const {
        return !addedPackages.empty() || !removedPackages.empty() || !changedPackages.empty();
    }

    std::string toString() const;
};

class Exporter {
public:
    // CSV Export (Strict RFC 4180 with spreadsheet formula injection prevention)
    static std::string exportPlanToCsv(const InstallationPlan& plan);
    static std::string exportHistoryToCsv(const std::vector<TransactionRecord>& history);

    // JSON Export (RFC 8259 compliant structured output)
    static std::string exportResolutionToJson(const ResolutionResult& result);
    static std::string exportHistoryToJson(const std::vector<TransactionRecord>& history);

    // Configuration Comparison (A10)
    static ConfigDiff compareConfigurations(
        const std::map<std::string, SemVer>& configA,
        const std::map<std::string, SemVer>& configB
    );

    // Lockfile Generator & Parser (E1)
    static std::string generateLockfile(
        const std::map<std::string, SemVer>& resolvedVersions,
        const InstallationPlan& plan,
        const Registry& registry
    );
};

} // namespace DepResolver

