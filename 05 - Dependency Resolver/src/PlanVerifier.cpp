#include "PlanVerifier.h"
#include <unordered_map>

namespace DepResolver {

PlanVerificationReport PlanVerifier::verify(const DependencyGraph& graph, const InstallationPlan& plan) {
    PlanVerificationReport report;
    std::unordered_map<std::string, size_t> packagePosition;

    // Check 1: No duplicates and record position of each package
    for (size_t i = 0; i < plan.steps.size(); ++i) {
        const auto& pkgName = plan.steps[i].packageName;
        if (packagePosition.find(pkgName) != packagePosition.end()) {
            report.isValid = false;
            report.errors.push_back("Duplicate package in installation plan: " + pkgName);
        } else {
            packagePosition[pkgName] = i;
        }
    }

    // Check 2: All nodes in the graph must be accounted for
    for (size_t u = 0; u < graph.nodeCount(); ++u) {
        const auto& name = graph.getNodeName(static_cast<NodeId>(u));
        if (packagePosition.find(name) == packagePosition.end()) {
            report.isValid = false;
            report.errors.push_back("Package '" + name + "' is present in graph but missing from plan");
        }
    }

    // Check 3: For each package, all of its forward dependencies MUST appear earlier in the plan
    for (size_t i = 0; i < plan.steps.size(); ++i) {
        const auto& pkgName = plan.steps[i].packageName;
        auto uOpt = graph.findNodeId(pkgName);
        if (!uOpt.has_value()) continue;

        NodeId u = uOpt.value();
        for (const auto& depEdge : graph.getDependencies(u)) {
            const auto& depName = graph.getNodeName(depEdge.target);
            auto posIt = packagePosition.find(depName);
            if (posIt == packagePosition.end()) {
                report.isValid = false;
                report.errors.push_back("Package '" + pkgName + "' requires missing dependency '" + depName + "'");
            } else if (posIt->second >= i) {
                report.isValid = false;
                report.errors.push_back(
                    "Ordering violation: Package '" + pkgName + "' (step " + std::to_string(i) +
                    ") is scheduled BEFORE its dependency '" + depName + "' (step " + std::to_string(posIt->second) + ")"
                );
            }
        }
    }

    return report;
}

} // namespace DepResolver

