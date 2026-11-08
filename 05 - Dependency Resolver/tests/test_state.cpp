#include "TestRunner.h"
#include "StateManager.h"

using namespace DepResolver;

TEST_CASE(test_orphan_detection_and_autoremove) {
    Registry reg;
    reg.registerPackage("web");
    reg.registerVersion("web", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("auth", Constraint::any())
    }));

    reg.registerPackage("auth");
    reg.registerVersion("auth", PackageVersion(SemVer::parse("1.0.0").value()));

    reg.registerPackage("old_analytics");
    reg.registerVersion("old_analytics", PackageVersion(SemVer::parse("0.9.0").value()));

    InstalledState state;
    // 'web' is explicit
    state.recordInstall(InstalledPackage("web", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));
    // 'auth' is installed as dependency of web
    state.recordInstall(InstalledPackage("auth", SemVer::parse("1.0.0").value(), InstallReason::DEPENDENCY));
    // 'old_analytics' was installed as dependency of something previously deleted
    state.recordInstall(InstalledPackage("old_analytics", SemVer::parse("0.9.0").value(), InstallReason::DEPENDENCY));

    auto orphans = StateManager::findOrphans(state, reg);
    ASSERT_EQ(orphans.size(), 1u);
    ASSERT_EQ(orphans[0], std::string("old_analytics"));

    // Auto-remove
    auto removed = StateManager::autoRemoveOrphans(state, reg);
    ASSERT_EQ(removed.size(), 1u);
    ASSERT_FALSE(state.isInstalled("old_analytics"));
    ASSERT_TRUE(state.isInstalled("web"));
    ASSERT_TRUE(state.isInstalled("auth"));
}

TEST_CASE(test_unused_package_detection) {
    Registry reg;
    reg.registerPackage("used_app");
    reg.registerVersion("used_app", PackageVersion(SemVer::parse("1.0.0").value()));

    reg.registerPackage("unused_tool");
    reg.registerVersion("unused_tool", PackageVersion(SemVer::parse("2.0.0").value()));

    InstalledState state;
    state.recordInstall(InstalledPackage("used_app", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));

    auto unused = StateManager::findUnused(state, reg);
    ASSERT_EQ(unused.size(), 1u);
    ASSERT_EQ(unused[0], std::string("unused_tool"));
}

TEST_CASE(test_transition_analysis_and_major_version_warning) {
    InstalledState state;
    state.recordInstall(InstalledPackage("libx", SemVer::parse("1.5.0").value(), InstallReason::EXPLICIT));

    std::map<std::string, SemVer> target;
    target["libx"] = SemVer::parse("2.0.0").value(); // Major upgrade!
    target["liby"] = SemVer::parse("1.0.0").value(); // New install

    auto transitions = StateManager::analyzeTransitions(state, target);
    ASSERT_EQ(transitions.size(), 2u);

    // libx transition
    ASSERT_EQ(transitions[0].packageName, std::string("libx"));
    ASSERT_EQ(transitions[0].type, TransitionType::UPGRADE);
    ASSERT_TRUE(transitions[0].isMajorChange);
    ASSERT_FALSE(transitions[0].warning.empty());

    // liby transition
    ASSERT_EQ(transitions[1].packageName, std::string("liby"));
    ASSERT_EQ(transitions[1].type, TransitionType::NEW_INSTALL);
    ASSERT_FALSE(transitions[1].isMajorChange);
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

