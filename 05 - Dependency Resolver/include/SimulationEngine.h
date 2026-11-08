#pragma once

#include "Common.h"
#include "SemVer.h"
#include "TopologicalSorter.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace DepResolver {

enum class SimStepType {
    FETCH = 0,
    VERIFY,
    UNPACK,
    CONFIGURE,
    REGISTER
};

inline const char* simStepToString(SimStepType step) {
    switch (step) {
        case SimStepType::FETCH: return "FETCH";
        case SimStepType::VERIFY: return "VERIFY";
        case SimStepType::UNPACK: return "UNPACK";
        case SimStepType::CONFIGURE: return "CONFIGURE";
        case SimStepType::REGISTER: return "REGISTER";
        default: return "UNKNOWN";
    }
}

enum class SimStepOutcome {
    SUCCESS = 0,
    FAILURE,
    SKIPPED
};

inline const char* simStepOutcomeToString(SimStepOutcome outcome) {
    switch (outcome) {
        case SimStepOutcome::SUCCESS: return "SUCCESS";
        case SimStepOutcome::FAILURE: return "FAILURE";
        case SimStepOutcome::SKIPPED: return "SKIPPED";
        default: return "UNKNOWN";
    }
}

struct SimLogEntry {
    uint64_t relativeTimeMs{0};
    size_t planStepOrder{0};
    std::string packageName;
    std::string version;
    SimStepType step{SimStepType::FETCH};
    SimStepOutcome outcome{SimStepOutcome::SUCCESS};
    std::string message;

    std::string toString() const;
};

// ============================================================================
// Deterministic Failure Injection Mechanism
// ============================================================================
struct FailureInjectionRule {
    std::string targetPackage;
    std::optional<SimStepType> targetStep;
    size_t targetStepIndex{SIZE_MAX};
    bool shouldFailInverseAction{false}; // For testing ROLLBACK_INCOMPLETE
    std::string errorMessage{"Injected simulated hardware/network fault"};
};

class SimulationEngine {
private:
    std::vector<FailureInjectionRule> failureRules_;
    uint64_t simulatedClockMs_{0};

public:
    SimulationEngine() = default;

    void addFailureRule(FailureInjectionRule rule) {
        failureRules_.push_back(std::move(rule));
    }

    void clearRules() { failureRules_.clear(); }

    bool shouldFail(const std::string& pkg, SimStepType step, size_t stepIndex, std::string& outMsg) const;
    bool shouldFailInverse(const std::string& pkg) const;

    void advanceClock(uint64_t ms) { simulatedClockMs_ += ms; }
    uint64_t clockMs() const { return simulatedClockMs_; }
};

} // namespace DepResolver

