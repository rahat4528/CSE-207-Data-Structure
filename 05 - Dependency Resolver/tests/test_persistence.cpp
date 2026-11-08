#include "TestRunner.h"
#include "Persistence.h"

using namespace DepResolver;

TEST_CASE(test_registry_save_and_load_roundtrip) {
    Registry reg;
    reg.registerPackage("react", "Declarative UI");
    reg.registerVersion("react", PackageVersion(SemVer::parse("18.2.0").value()));

    reg.registerPackage("nextjs", "Fullstack React Framework");
    reg.registerVersion("nextjs", PackageVersion(SemVer::parse("14.0.0").value(), {
        DependencyDecl("react", Constraint::parse("^18.0.0").value())
    }));

    std::string testFile = "bin/test_reg.manifest";
    auto saveRes = Persistence::saveRegistry(reg, testFile);
    ASSERT_TRUE(saveRes.isOk());

    auto loadRes = Persistence::loadRegistry(testFile);
    ASSERT_TRUE(loadRes.isOk());

    const Registry& loaded = loadRes.value();
    ASSERT_EQ(loaded.packageCount(), 2u);
    ASSERT_TRUE(loaded.hasPackage("react"));
    ASSERT_TRUE(loaded.hasPackage("nextjs"));

    const auto* nextPkg = loaded.getPackage("nextjs");
    ASSERT_TRUE(nextPkg != nullptr);
    ASSERT_EQ(nextPkg->versions().size(), 1u);
    ASSERT_EQ(nextPkg->versions()[0].dependencies.size(), 1u);
    ASSERT_EQ(nextPkg->versions()[0].dependencies[0].packageName, std::string("react"));
}

TEST_CASE(test_installed_state_roundtrip) {
    InstalledState state;
    state.recordInstall(InstalledPackage("express", SemVer::parse("4.18.2").value(), InstallReason::EXPLICIT, "2026-11-07", 101));
    state.recordInstall(InstalledPackage("cookie-parser", SemVer::parse("1.4.6").value(), InstallReason::DEPENDENCY, "2026-11-07", 101));

    std::string testFile = "bin/test_state.state";
    auto saveRes = Persistence::saveInstalledState(state, testFile);
    ASSERT_TRUE(saveRes.isOk());

    auto loadRes = Persistence::loadInstalledState(testFile);
    ASSERT_TRUE(loadRes.isOk());

    const auto& loaded = loadRes.value();
    ASSERT_EQ(loaded.count(), 2u);
    ASSERT_TRUE(loaded.isInstalled("express"));
    ASSERT_TRUE(loaded.isInstalled("cookie-parser"));
    ASSERT_EQ(loaded.get("express")->version.toString(), std::string("4.18.2"));
}

TEST_CASE(test_crash_recovery_from_journal) {
    std::string journalFile = "bin/test_crash.journal";
    Persistence::clearJournal(journalFile);

    InstalledState state;
    state.recordInstall(InstalledPackage("half_baked_pkg", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));

    // Simulate crash: TX_BEGIN and TX_INSTALLED logged, but application crashed before TX_COMMIT!
    Persistence::appendJournal(journalFile, "TX_BEGIN 42");
    Persistence::appendJournal(journalFile, "TX_INSTALLED half_baked_pkg");

    // Run recovery
    auto recRes = Persistence::performCrashRecovery(journalFile, state);
    ASSERT_TRUE(recRes.isOk());
    ASSERT_TRUE(recRes.value()); // Recovery happened!

    // 'half_baked_pkg' must be removed from state!
    ASSERT_FALSE(state.isInstalled("half_baked_pkg"));
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

