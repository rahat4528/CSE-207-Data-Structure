#pragma once

#include "Common.h"
#include "Registry.h"
#include "DependencyGraph.h"
#include "TopologicalSorter.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace DepResolver {

enum class ResolutionStatus {
    RESOLVED = 0,
    UNSATISFIABLE,
    LIMIT_REACHED
};

inline const char* resolutionStatusToString(ResolutionStatus s) {
    switch (s) {
        case ResolutionStatus::RESOLVED: return "RESOLVED";
        case ResolutionStatus::UNSATISFIABLE: return "UNSATISFIABLE";
        case ResolutionStatus::LIMIT_REACHED: return "LIMIT_REACHED";
        default: return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, ResolutionStatus s) {
    return os << resolutionStatusToString(s);
}

struct ResolutionRequest {
    std::vector<DependencyDecl> rootRequirements;
    bool newestFirst{true}; // true = Newest / Highest satisfying; false = Minimal Version Selection
    size_t maxSteps{Limits::MAX_BACKTRACK_STEPS};
};

struct ResolutionResult {
    ResolutionStatus status{ResolutionStatus::UNSATISFIABLE};
    std::map<std::string, SemVer> selectedVersions; // Sorted by package name for determinism
    DependencyGraph resolvedGraph;
    InstallationPlan plan;
    std::vector<Diagnostic> diagnostics;
    size_t stepsTaken{0};

    bool isSuccess() const { return status == ResolutionStatus::RESOLVED; }
};

class Resolver {
public:
    // Main Entrypoint: Deterministic Backtracking Constraint Solver
    static ResolutionResult resolve(const Registry& registry, const ResolutionRequest& request);
};

} // namespace DepResolver

