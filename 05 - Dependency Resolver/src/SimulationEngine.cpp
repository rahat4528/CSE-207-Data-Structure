#include "SimulationEngine.h"
#include <sstream>

namespace DepResolver {

std::string SimLogEntry::toString() const {
    std::ostringstream ss;
    ss << "[" << relativeTimeMs << "ms] [Step " << planStepOrder << "] "
       << packageName << "@" << version << " -> "
       << simStepToString(step) << ": " << simStepOutcomeToString(outcome);
    if (!message.empty()) {
        ss << " (" << Validator::sanitizeText(message) << ")";
    }
    return ss.str();
}

bool SimulationEngine::shouldFail(const std::string& pkg, SimStepType step, size_t stepIndex, std::string& outMsg) const {
    for (const auto& rule : failureRules_) {
        if (rule.shouldFailInverseAction) continue; // Dedicated inverse action rule
        if (!rule.targetPackage.empty() && rule.targetPackage != pkg) continue;
        if (rule.targetStep.has_value() && rule.targetStep.value() != step) continue;
        if (rule.targetStepIndex != SIZE_MAX && rule.targetStepIndex != stepIndex) continue;

        outMsg = rule.errorMessage;
        return true;
    }
    return false;
}

bool SimulationEngine::shouldFailInverse(const std::string& pkg) const {
    for (const auto& rule : failureRules_) {
        if (rule.targetPackage == pkg && rule.shouldFailInverseAction) {
            return true;
        }
    }
    return false;
}

} // namespace DepResolver

