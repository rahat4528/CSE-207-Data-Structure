#include "TransactionManager.h"
#include <iostream>

namespace DepResolver {

TransactionExecutionResult TransactionManager::executePlan(
    InstalledState& state,
    const InstallationPlan& plan,
    const std::map<std::string, SemVer>& resolvedVersions,
    SimulationEngine& simEngine,
    InstallReason defaultReason) 
{
    uint64_t txId = nextTransactionId_++;
    TransactionExecutionResult result;
    result.transactionId = txId;
    result.status = TransactionStatus::ACTIVE;

    // LIFO Rollback Stack
    std::vector<RollbackOp> rollbackStack;

    // Ordered steps inside plan
    bool hasFailure = false;
    std::string failedPackage;
    std::string failureReason;

    for (size_t stepIdx = 0; stepIdx < plan.steps.size(); ++stepIdx) {
        const auto& step = plan.steps[stepIdx];
        const std::string& pkgName = step.packageName;
        SemVer version = resolvedVersions.at(pkgName);

        // Record pre-existing state for this package to prepare inverse rollback op
        const auto* existing = state.get(pkgName);
        RollbackOp inverseOp;
        if (existing) {
            inverseOp = RollbackOp(InverseOpType::RESTORE_PREVIOUS, pkgName, existing->version, existing->reason);
        } else {
            inverseOp = RollbackOp(InverseOpType::UNINSTALL_NEW, pkgName);
        }

        // Simulate package lifecycle: FETCH -> VERIFY -> UNPACK -> CONFIGURE -> REGISTER
        const SimStepType lifecycle[] = {
            SimStepType::FETCH,
            SimStepType::VERIFY,
            SimStepType::UNPACK,
            SimStepType::CONFIGURE,
            SimStepType::REGISTER
        };

        for (SimStepType currentStep : lifecycle) {
            simEngine.advanceClock(20); // deterministic simulated step duration
            std::string errMsg;

            if (simEngine.shouldFail(pkgName, currentStep, stepIdx, errMsg)) {
                // Failure injected/triggered!
                SimLogEntry log;
                log.relativeTimeMs = simEngine.clockMs();
                log.planStepOrder = stepIdx;
                log.packageName = pkgName;
                log.version = version.toString();
                log.step = currentStep;
                log.outcome = SimStepOutcome::FAILURE;
                log.message = errMsg;
                result.logs.push_back(std::move(log));

                hasFailure = true;
                failedPackage = pkgName;
                failureReason = errMsg;
                break;
            } else {
                SimLogEntry log;
                log.relativeTimeMs = simEngine.clockMs();
                log.planStepOrder = stepIdx;
                log.packageName = pkgName;
                log.version = version.toString();
                log.step = currentStep;
                log.outcome = SimStepOutcome::SUCCESS;
                log.message = "OK";
                result.logs.push_back(std::move(log));
            }
        }

        if (hasFailure) {
            break;
        }

        // Successfully installed package: record in state and push inverse op to rollback stack
        state.recordInstall(InstalledPackage(
            pkgName, version, 
            step.reason.find("Directly requested") != std::string::npos ? InstallReason::EXPLICIT : defaultReason,
            std::to_string(simEngine.clockMs()), txId
        ));
        result.successfullyInstalled.push_back(pkgName);
        rollbackStack.push_back(std::move(inverseOp));
    }

    if (!hasFailure) {
        // Transaction committed successfully!
        result.status = TransactionStatus::COMMITTED;
        result.summary = "Transaction " + std::to_string(txId) + " committed successfully (" + 
                         std::to_string(result.successfullyInstalled.size()) + " packages installed).";
        return result;
    }

    // Transaction failed: UNWIND ROLLBACK STACK (LIFO Order)
    bool rollbackIncomplete = false;

    while (!rollbackStack.empty()) {
        RollbackOp op = rollbackStack.back();
        rollbackStack.pop_back();

        // Check if inverse action fails (simulated)
        if (simEngine.shouldFailInverse(op.packageName)) {
            rollbackIncomplete = true;
            result.inconsistentPackages.push_back(op.packageName);
            continue;
        }

        if (op.type == InverseOpType::UNINSTALL_NEW) {
            state.recordUninstall(op.packageName);
            result.rolledBackPackages.push_back(op.packageName);
        } else if (op.type == InverseOpType::RESTORE_PREVIOUS) {
            state.recordInstall(InstalledPackage(
                op.packageName, op.previousVersion, op.previousReason, 
                std::to_string(simEngine.clockMs()), txId
            ));
            result.rolledBackPackages.push_back(op.packageName);
        }
    }

    if (rollbackIncomplete) {
        result.status = TransactionStatus::ROLLBACK_INCOMPLETE;
        result.summary = "Transaction " + std::to_string(txId) + " aborted due to failure in '" + failedPackage + 
                         "': " + failureReason + ". ROLLBACK INCOMPLETE: Inconsistent packages remaining.";
    } else {
        result.status = TransactionStatus::ROLLED_BACK;
        result.summary = "Transaction " + std::to_string(txId) + " aborted due to failure in '" + failedPackage + 
                         "': " + failureReason + ". Rolled back completely to pre-transaction state.";
    }

    return result;
}

} // namespace DepResolver

