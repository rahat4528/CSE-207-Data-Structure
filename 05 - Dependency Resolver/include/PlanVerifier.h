#pragma once

#include "Common.h"
#include "TopologicalSorter.h"
#include "DependencyGraph.h"
#include <unordered_set>
#include <string>

namespace DepResolver {

struct PlanVerificationReport {
    bool isValid{true};
    std::vector<std::string> errors;

    std::string reportString() const {
        if (isValid) return "Plan is VALID";
        std::string s = "Plan verification FAILED:\n";
        for (const auto& err : errors) {
            s += "  - " + err + "\n";
        }
        return s;
    }
};

class PlanVerifier {
public:
    // Strictly verifies that:
    // 1. Every package appears exactly once (no duplicates).
    // 2. For every package P in the plan, all dependencies of P appear STRICTLY EARLIER in the plan.
    // 3. No missing dependencies.
    // 4. Graph represented by the plan is strictly acyclic.
    static PlanVerificationReport verify(const DependencyGraph& graph, const InstallationPlan& plan);
};

} // namespace DepResolver

