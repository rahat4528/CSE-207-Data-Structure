#include "TestRunner.h"
#include "Exporter.h"
#include "Visualizer.h"

using namespace DepResolver;

TEST_CASE(test_csv_export_and_formula_injection_prevention) {
    InstallationPlan plan;
    plan.steps = {
        {0, "=SUM(A1:A10)", "1.0.0", 0, "Formula test", {}},
        {1, "safe_pkg,with_comma", "2.0.0", 1, "Direct", {}}
    };

    std::string csv = Exporter::exportPlanToCsv(plan);
    ASSERT_TRUE(csv.find("'=SUM(A1:A10)") != std::string::npos); // Formula neutralized!
    ASSERT_TRUE(csv.find("\"safe_pkg,with_comma\"") != std::string::npos); // Quoted!
}

TEST_CASE(test_config_comparison_diff) {
    std::map<std::string, SemVer> configA;
    configA["common"] = SemVer::parse("1.0.0").value();
    configA["removed_pkg"] = SemVer::parse("1.0.0").value();

    std::map<std::string, SemVer> configB;
    configB["common"] = SemVer::parse("2.0.0").value(); // Upgraded (Major change!)
    configB["added_pkg"] = SemVer::parse("1.0.0").value();

    auto diff = Exporter::compareConfigurations(configA, configB);
    ASSERT_TRUE(diff.hasDifferences());
    ASSERT_EQ(diff.addedPackages.size(), 1u);
    ASSERT_EQ(diff.addedPackages[0], std::string("added_pkg"));
    ASSERT_EQ(diff.removedPackages.size(), 1u);
    ASSERT_EQ(diff.removedPackages[0], std::string("removed_pkg"));
    ASSERT_EQ(diff.changedPackages.size(), 1u);
    ASSERT_TRUE(diff.changedPackages[0].isMajorChange);
}

TEST_CASE(test_lockfile_generation) {
    Registry reg;
    reg.registerPackage("core");
    reg.registerVersion("core", PackageVersion(SemVer::parse("1.2.0").value(), {}, "sha256-abc1234"));

    std::map<std::string, SemVer> resolved;
    resolved["core"] = SemVer::parse("1.2.0").value();

    InstallationPlan plan;
    plan.steps = {{0, "core", "1.2.0", 0, "root", {}}};
    plan.totalLevels = 1;

    std::string lockfile = Exporter::generateLockfile(resolved, plan, reg);
    ASSERT_TRUE(lockfile.find("# DEPENDENCY_RESOLVER_LOCKFILE_V1") != std::string::npos);
    ASSERT_TRUE(lockfile.find("core = 1.2.0") != std::string::npos);
    ASSERT_TRUE(lockfile.find("checksum=sha256-abc1234") != std::string::npos);
}

TEST_CASE(test_ascii_tree_and_svg_visualization) {
    DependencyGraph graph;
    graph.addEdge("app", "libA");
    graph.addEdge("app", "libB");
    graph.addEdge("libA", "core");
    graph.addEdge("libB", "core"); // Diamond shared dependency

    std::string tree = Visualizer::renderAsciiTree(graph, "app");
    ASSERT_TRUE(tree.find("app") != std::string::npos);
    ASSERT_TRUE(tree.find("[shared/deduped]") != std::string::npos); // Shared dependency marked!

    std::string svg = Visualizer::renderSvg(graph, "Test Architecture");
    ASSERT_TRUE(svg.find("<svg") != std::string::npos);
    ASSERT_TRUE(svg.find("</svg>") != std::string::npos);
    ASSERT_TRUE(svg.find("Test Architecture") != std::string::npos);
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

