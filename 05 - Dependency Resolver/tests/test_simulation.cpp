#include "TestRunner.h"
#include "SimulationEngine.h"
#include "TransactionManager.h"
#include "HistoryManager.h"
#include "Resolver.h"

using namespace DepResolver;

TEST_CASE(test_successful_simulation_and_history) {
    InstalledState state;
    SimulationEngine sim;
    TransactionManager txMgr;
    HistoryManager hist;

    InstallationPlan plan;
    plan.steps = {
        {0, "lib_core", "1.0.0", 0, "Required by app", {}},
        {1, "app", "2.0.0", 1, "Directly requested", {"lib_core"}}
    };

    std::map<std::string, SemVer> versions;
    versions["lib_core"] = SemVer::parse("1.0.0").value();
    versions["app"] = SemVer::parse("2.0.0").value();

    auto txRes = txMgr.executePlan(state, plan, versions, sim);
    ASSERT_TRUE(txRes.isSuccess());
    ASSERT_EQ(txRes.status, TransactionStatus::COMMITTED);
    ASSERT_EQ(txRes.successfullyInstalled.size(), 2u);

    // Verify state
    ASSERT_TRUE(state.isInstalled("lib_core"));
    ASSERT_TRUE(state.isInstalled("app"));
    ASSERT_EQ(state.get("lib_core")->version.toString(), std::string("1.0.0"));
    ASSERT_EQ(state.get("app")->version.toString(), std::string("2.0.0"));

    // Record to history list
    hist.append({txRes.transactionId, "2026-11-07T13:30:00", "install", txRes.status, txRes.successfullyInstalled, txRes.summary, txRes.logs});
    ASSERT_EQ(hist.size(), 1u);

    auto found = hist.findById(txRes.transactionId);
    ASSERT_TRUE(found != nullptr);
    ASSERT_EQ(found->affectedPackages.size(), 2u);
}

TEST_CASE(test_failure_triggers_clean_rollback_to_pre_state) {
    InstalledState state;
    // Initial pre-state: 'base_lib' 1.0.0 is already installed
    state.recordInstall(InstalledPackage("base_lib", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));

    SimulationEngine sim;
    // Inject simulated network/unpack failure on 'faulty_pkg'
    FailureInjectionRule rule;
    rule.targetPackage = "faulty_pkg";
    rule.targetStep = SimStepType::UNPACK;
    rule.errorMessage = "Disk space exhausted during unpack";
    sim.addFailureRule(rule);

    TransactionManager txMgr;

    // Plan attempts to install 'dep1', upgrade 'base_lib' to 2.0.0, and install 'faulty_pkg'
    InstallationPlan plan;
    plan.steps = {
        {0, "dep1", "1.0.0", 0, "dependency", {}},
        {1, "base_lib", "2.0.0", 0, "upgrade", {}},
        {2, "faulty_pkg", "1.0.0", 1, "Directly requested", {"dep1", "base_lib"}}
    };

    std::map<std::string, SemVer> versions;
    versions["dep1"] = SemVer::parse("1.0.0").value();
    versions["base_lib"] = SemVer::parse("2.0.0").value();
    versions["faulty_pkg"] = SemVer::parse("1.0.0").value();

    auto txRes = txMgr.executePlan(state, plan, versions, sim);
    ASSERT_FALSE(txRes.isSuccess());
    ASSERT_EQ(txRes.status, TransactionStatus::ROLLED_BACK);

    // Verify EXACT restoration to pre-transaction state:
    // 1. 'dep1' must be uninstalled!
    ASSERT_FALSE(state.isInstalled("dep1"));
    // 2. 'faulty_pkg' must NOT be installed!
    ASSERT_FALSE(state.isInstalled("faulty_pkg"));
    // 3. 'base_lib' must be restored to version 1.0.0!
    ASSERT_TRUE(state.isInstalled("base_lib"));
    ASSERT_EQ(state.get("base_lib")->version.toString(), std::string("1.0.0"));
    ASSERT_EQ(state.count(), 1u);
}

TEST_CASE(test_failed_inverse_action_produces_rollback_incomplete) {
    InstalledState state;
    SimulationEngine sim;

    // Inject failure on 'pkg_fail'
    FailureInjectionRule failRule;
    failRule.targetPackage = "pkg_fail";
    failRule.targetStep = SimStepType::CONFIGURE;
    sim.addFailureRule(failRule);

    // Also inject inverse action failure on 'pkg_locked'
    FailureInjectionRule lockedRule;
    lockedRule.targetPackage = "pkg_locked";
    lockedRule.shouldFailInverseAction = true;
    sim.addFailureRule(lockedRule);

    TransactionManager txMgr;

    InstallationPlan plan;
    plan.steps = {
        {0, "pkg_locked", "1.0.0", 0, "dep", {}},
        {1, "pkg_fail", "1.0.0", 1, "req", {"pkg_locked"}}
    };

    std::map<std::string, SemVer> versions;
    versions["pkg_locked"] = SemVer::parse("1.0.0").value();
    versions["pkg_fail"] = SemVer::parse("1.0.0").value();

    auto txRes = txMgr.executePlan(state, plan, versions, sim);
    ASSERT_FALSE(txRes.isSuccess());
    ASSERT_EQ(txRes.status, TransactionStatus::ROLLBACK_INCOMPLETE);
    ASSERT_EQ(txRes.inconsistentPackages.size(), 1u);
    ASSERT_EQ(txRes.inconsistentPackages[0], std::string("pkg_locked"));
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

