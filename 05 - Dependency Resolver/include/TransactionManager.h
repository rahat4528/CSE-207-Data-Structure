#pragma once

#include "Common.h"
#include "StateManager.h"
#include "SimulationEngine.h"
#include "TopologicalSorter.h"
#include <string>
#include <vector>
#include <stack>
#include <memory>

namespace DepResolver {

enum class TransactionStatus {
    ACTIVE = 0,
    COMMITTED,
    ROLLED_BACK,
    ROLLBACK_INCOMPLETE
};

inline const char* transactionStatusToString(TransactionStatus s) {
    switch (s) {
        case TransactionStatus::ACTIVE: return "ACTIVE";
        case TransactionStatus::COMMITTED: return "COMMITTED";
        case TransactionStatus::ROLLED_BACK: return "ROLLED_BACK";
        case TransactionStatus::ROLLBACK_INCOMPLETE: return "ROLLBACK_INCOMPLETE";
        default: return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, TransactionStatus s) {
    return os << transactionStatusToString(s);
}

enum class InverseOpType {
    UNINSTALL_NEW = 0,
    RESTORE_PREVIOUS
};

struct RollbackOp {
    InverseOpType type{InverseOpType::UNINSTALL_NEW};
    std::string packageName;
    SemVer previousVersion{0, 0, 0};
    InstallReason previousReason{InstallReason::EXPLICIT};

    RollbackOp() = default;
    RollbackOp(InverseOpType t, std::string pkg, SemVer prevVer = {0, 0, 0}, InstallReason prevReason = InstallReason::EXPLICIT)
        : type(t), packageName(std::move(pkg)), previousVersion(prevVer), previousReason(prevReason) {}
};

struct TransactionExecutionResult {
    uint64_t transactionId{0};
    TransactionStatus status{TransactionStatus::ACTIVE};
    std::vector<SimLogEntry> logs;
    std::vector<std::string> successfullyInstalled;
    std::vector<std::string> rolledBackPackages;
    std::vector<std::string> inconsistentPackages; // populated only if ROLLBACK_INCOMPLETE
    std::string summary;

    bool isSuccess() const { return status == TransactionStatus::COMMITTED; }
};

class TransactionManager {
private:
    uint64_t nextTransactionId_{1};

public:
    TransactionManager() = default;

    // Execute installation plan inside an atomic transaction with LIFO rollback stack
    TransactionExecutionResult executePlan(
        InstalledState& state,
        const InstallationPlan& plan,
        const std::map<std::string, SemVer>& resolvedVersions,
        SimulationEngine& simEngine,
        InstallReason defaultReason = InstallReason::EXPLICIT
    );
};

} // namespace DepResolver

