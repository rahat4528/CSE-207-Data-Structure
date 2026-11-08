#include "Resolver.h"
#include "CycleDetector.h"
#include "PlanVerifier.h"
#include <algorithm>

namespace DepResolver {

namespace {

struct SolverState {
    std::map<std::string, SemVer> chosenVersions;
    std::map<std::string, Constraint> accumulatedConstraints;
    std::map<std::string, std::vector<std::string>> dependencyChains; // path from root to pkg
    std::vector<std::string> pendingPackages;
    std::vector<Diagnostic> diagnostics;
    size_t steps{0};
    bool limitReached{false};
};

bool solveRecursive(const Registry& registry, const ResolutionRequest& request, SolverState& state) {
    if (++state.steps > request.maxSteps) {
        state.limitReached = true;
        state.diagnostics.push_back(Diagnostic(
            ErrorCode::LIMIT_REACHED, "", 
            "Resolution step limit exceeded (" + std::to_string(request.maxSteps) + " steps). Search abandoned."
        ));
        return false;
    }

    // Base case: all needed packages have been assigned a version
    if (state.pendingPackages.empty()) {
        return true;
    }

    // Deterministic selection: pick the next package from pending
    std::string currentPkg = state.pendingPackages.back();
    state.pendingPackages.pop_back();

    // If current package was already assigned a version (from earlier branch), verify and proceed
    if (state.chosenVersions.find(currentPkg) != state.chosenVersions.end()) {
        const auto& chosenVer = state.chosenVersions[currentPkg];
        const auto& constr = state.accumulatedConstraints[currentPkg];
        if (!constr.satisfies(chosenVer)) {
            // Inconsistency detected
            state.pendingPackages.push_back(currentPkg);
            return false;
        }
        bool ok = solveRecursive(registry, request, state);
        if (!ok) {
            state.pendingPackages.push_back(currentPkg);
        }
        return ok;
    }

    const Package* pkg = registry.getPackage(currentPkg);
    if (!pkg) {
        state.diagnostics.push_back(Diagnostic(
            ErrorCode::MISSING_PACKAGE, currentPkg, 
            "Package '" + currentPkg + "' does not exist in registry",
            state.dependencyChains[currentPkg]
        ));
        state.pendingPackages.push_back(currentPkg);
        return false;
    }

    const Constraint& currentConstraint = state.accumulatedConstraints[currentPkg];
    auto candidateVersions = pkg->getMatchingVersions(currentConstraint, request.newestFirst);

    if (candidateVersions.empty()) {
        state.diagnostics.push_back(Diagnostic(
            ErrorCode::NO_MATCHING_VERSION, currentPkg,
            "No version of '" + currentPkg + "' satisfies constraint: " + currentConstraint.toString(),
            state.dependencyChains[currentPkg]
        ));
        state.pendingPackages.push_back(currentPkg);
        return false;
    }

    // Try each candidate version in preference order (newest first by default)
    for (const auto* cand : candidateVersions) {
        // Snapshot state for backtracking
        auto prevConstraints = state.accumulatedConstraints;
        auto prevChains = state.dependencyChains;
        auto prevPending = state.pendingPackages;

        bool conflict = false;
        std::vector<std::string> newlyAddedPending;

        // Propagate dependencies of this candidate version
        for (const auto& dep : cand->dependencies) {
            // We check only RUNTIME dependencies for core installation resolution
            if (dep.kind != DependencyKind::RUNTIME_DEP) continue;

            const std::string& depName = dep.packageName;

            // Check if dependency package exists in registry
            if (!registry.hasPackage(depName)) {
                conflict = true;
                auto chain = state.dependencyChains[currentPkg];
                chain.push_back(depName);
                state.diagnostics.push_back(Diagnostic(
                    ErrorCode::MISSING_PACKAGE, depName,
                    "Required dependency '" + depName + "' is missing from registry",
                    chain
                ));
                break;
            }

            // Update dependency chain for diagnostics
            if (state.dependencyChains.find(depName) == state.dependencyChains.end()) {
                auto chain = state.dependencyChains[currentPkg];
                chain.push_back(depName);
                state.dependencyChains[depName] = std::move(chain);
            }

            // Check against already chosen version if present
            auto chosenIt = state.chosenVersions.find(depName);
            if (chosenIt != state.chosenVersions.end()) {
                if (!dep.constraint.satisfies(chosenIt->second)) {
                    conflict = true;
                    state.diagnostics.push_back(Diagnostic(
                        ErrorCode::VERSION_CONFLICT, depName,
                        "Version conflict on '" + depName + "': already chosen " + chosenIt->second.toString() +
                        " does not satisfy " + dep.constraint.toString() + " required by " + currentPkg + " " + cand->version.toString(),
                        state.dependencyChains[depName]
                    ));
                    break;
                }
            } else {
                // Intersect with existing constraints on depName
                auto constrIt = state.accumulatedConstraints.find(depName);
                if (constrIt != state.accumulatedConstraints.end()) {
                    auto inter = constrIt->second.intersect(dep.constraint);
                    if (!inter.has_value()) {
                        conflict = true;
                        state.diagnostics.push_back(Diagnostic(
                            ErrorCode::VERSION_CONFLICT, depName,
                            "Direct version constraint conflict on '" + depName + "': [" + 
                            constrIt->second.toString() + "] and [" + dep.constraint.toString() + "] are disjoint",
                            state.dependencyChains[depName]
                        ));
                        break;
                    }
                    constrIt->second = inter.value();
                } else {
                    state.accumulatedConstraints[depName] = dep.constraint;
                    // Add to pending
                    state.pendingPackages.push_back(depName);
                    newlyAddedPending.push_back(depName);
                }
            }
        }

        if (!conflict) {
            state.chosenVersions[currentPkg] = cand->version;

            // Recurse to next package
            if (solveRecursive(registry, request, state)) {
                return true;
            }

            // Backtrack chosen version
            state.chosenVersions.erase(currentPkg);
        }

        // Restore state on backtrack
        state.accumulatedConstraints = std::move(prevConstraints);
        state.dependencyChains = std::move(prevChains);
        state.pendingPackages = std::move(prevPending);

        if (state.limitReached) return false;
    }

    state.pendingPackages.push_back(currentPkg);
    return false;
}

} // namespace

ResolutionResult Resolver::resolve(const Registry& registry, const ResolutionRequest& request) {
    ResolutionResult result;
    SolverState state;

    // Initialize root requirements
    for (const auto& req : request.rootRequirements) {
        state.accumulatedConstraints[req.packageName] = req.constraint;
        state.dependencyChains[req.packageName] = {req.packageName};
        state.pendingPackages.push_back(req.packageName);
    }

    // Sort pending packages in descending alphabetical order so popping gives alphabetical order
    std::sort(state.pendingPackages.rbegin(), state.pendingPackages.rend());

    bool solved = solveRecursive(registry, request, state);
    result.stepsTaken = state.steps;
    result.diagnostics = std::move(state.diagnostics);

    if (state.limitReached) {
        result.status = ResolutionStatus::LIMIT_REACHED;
        return result;
    }

    if (!solved) {
        result.status = ResolutionStatus::UNSATISFIABLE;
        return result;
    }

    // Build the resolved concrete dependency graph
    DependencyGraph resolvedGraph;
    for (const auto& [pkgName, ver] : state.chosenVersions) {
        resolvedGraph.getOrAddNode(pkgName);
    }

    for (const auto& [pkgName, ver] : state.chosenVersions) {
        const Package* p = registry.getPackage(pkgName);
        if (!p) continue;
        const PackageVersion* pv = p->findVersion(ver);
        if (!pv) continue;

        for (const auto& dep : pv->dependencies) {
            if (dep.kind == DependencyKind::RUNTIME_DEP) {
                resolvedGraph.addEdge(pkgName, dep.packageName, dep.constraint, dep.kind);
            }
        }
    }

    // Check for circular dependencies in the resolved graph
    auto cycleCheck = CycleDetector::detectCycle(resolvedGraph);
    if (cycleCheck.hasCycle) {
        result.status = ResolutionStatus::UNSATISFIABLE;
        result.diagnostics.push_back(Diagnostic(
            ErrorCode::CYCLE_DETECTED, cycleCheck.cyclePath.front(),
            cycleCheck.message, cycleCheck.cyclePath
        ));
        return result;
    }

    // Generate reproducible installation plan via Kahn's algorithm
    auto planRes = TopologicalSorter::sortKahn(resolvedGraph);
    if (planRes.isErr()) {
        result.status = ResolutionStatus::UNSATISFIABLE;
        result.diagnostics.push_back(planRes.error());
        return result;
    }

    InstallationPlan plan = planRes.value();
    // Fill in exact resolved versions in plan steps
    for (auto& step : plan.steps) {
        step.version = state.chosenVersions[step.packageName].toString();
    }

    // Independent Plan Verification
    auto report = PlanVerifier::verify(resolvedGraph, plan);
    if (!report.isValid) {
        result.status = ResolutionStatus::UNSATISFIABLE;
        for (const auto& err : report.errors) {
            result.diagnostics.push_back(Diagnostic(
                ErrorCode::PLAN_VERIFICATION_FAILED, "", err
            ));
        }
        return result;
    }

    result.status = ResolutionStatus::RESOLVED;
    result.selectedVersions = std::move(state.chosenVersions);
    result.resolvedGraph = std::move(resolvedGraph);
    result.plan = std::move(plan);
    return result;
}

} // namespace DepResolver

