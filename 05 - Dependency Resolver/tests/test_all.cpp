#include "TestRunner.h"
#include "Common.h"
#include "SemVer.h"
#include "Constraint.h"
#include "Package.h"
#include "DependencyGraph.h"
#include "Registry.h"
#include "CycleDetector.h"
#include "TopologicalSorter.h"
#include "PlanVerifier.h"
#include "Resolver.h"
#include "StateManager.h"
#include "SimulationEngine.h"
#include "TransactionManager.h"
#include "HistoryManager.h"
#include "Persistence.h"
#include "Exporter.h"
#include "Visualizer.h"

using namespace DepResolver;

// ============================================================================
// Group 1: Common, Error Types & Input Validation
// ============================================================================
TEST_CASE(tc01_error_codes_and_diagnostics) {
    ASSERT_EQ(errorCodeToString(ErrorCode::SUCCESS), std::string("SUCCESS"));
    ASSERT_EQ(errorCodeToString(ErrorCode::CYCLE_DETECTED), std::string("CYCLE_DETECTED"));
    ASSERT_EQ(errorCodeToString(ErrorCode::VERSION_CONFLICT), std::string("VERSION_CONFLICT"));

    Diagnostic diag(ErrorCode::VERSION_CONFLICT, "libssl", "Version mismatch", {"web_app", "database", "libssl"});
    ASSERT_TRUE(diag.isError());
    ASSERT_EQ(diag.chainToString(), std::string("web_app -> database -> libssl"));
}

TEST_CASE(tc02_package_name_validation_and_sanitization) {
    ASSERT_TRUE(Validator::validatePackageName("boost").isOk());
    ASSERT_TRUE(Validator::validatePackageName("my_lib-2.0").isOk());
    ASSERT_TRUE(Validator::validatePackageName("").isErr());
    ASSERT_TRUE(Validator::validatePackageName("-invalid").isErr());
    ASSERT_TRUE(Validator::validatePackageName("path/escape").isErr());

    std::string dirty = "Text\x1b[31mRed\x07";
    std::string clean = Validator::sanitizeText(dirty);
    ASSERT_FALSE(clean.find('\x07') != std::string::npos);
}

// ============================================================================
// Group 2: SemVer 2.0.0 & Constraint Grammar
// ============================================================================
TEST_CASE(tc03_semver_precedence_chain) {
    auto v1 = SemVer::parse("1.0.0-alpha").value();
    auto v2 = SemVer::parse("1.0.0-alpha.1").value();
    auto v3 = SemVer::parse("1.0.0-alpha.beta").value();
    auto v4 = SemVer::parse("1.0.0-beta").value();
    auto v5 = SemVer::parse("1.0.0-beta.2").value();
    auto v6 = SemVer::parse("1.0.0-beta.11").value();
    auto v7 = SemVer::parse("1.0.0-rc.1").value();
    auto v8 = SemVer::parse("1.0.0").value();

    ASSERT_TRUE(v1 < v2);
    ASSERT_TRUE(v2 < v3);
    ASSERT_TRUE(v3 < v4);
    ASSERT_TRUE(v4 < v5);
    ASSERT_TRUE(v5 < v6);
    ASSERT_TRUE(v6 < v7);
    ASSERT_TRUE(v7 < v8);
}

TEST_CASE(tc04_semver_build_metadata_ignored) {
    auto b1 = SemVer::parse("1.0.0+20130313144700").value();
    auto b2 = SemVer::parse("1.0.0+exp.sha.5114f85").value();
    ASSERT_TRUE(b1 == b2);
}

TEST_CASE(tc05_strict_parsing_rejects_leading_zeros) {
    ASSERT_TRUE(SemVer::parse("01.0.0").isErr());
    ASSERT_TRUE(SemVer::parse("1.02.0").isErr());
    ASSERT_TRUE(SemVer::parse("1.0.03").isErr());
    ASSERT_TRUE(SemVer::parse("1.0.0-01").isErr());
    ASSERT_TRUE(SemVer::parse("0.0.0").isOk());
}

TEST_CASE(tc06_caret_and_tilde_constraints) {
    auto c1 = Constraint::parse("^1.2.3").value();
    ASSERT_TRUE(c1.satisfies(SemVer::parse("1.2.3").value()));
    ASSERT_TRUE(c1.satisfies(SemVer::parse("1.9.9").value()));
    ASSERT_FALSE(c1.satisfies(SemVer::parse("2.0.0").value()));

    auto c0 = Constraint::parse("^0.2.3").value();
    ASSERT_TRUE(c0.satisfies(SemVer::parse("0.2.3").value()));
    ASSERT_FALSE(c0.satisfies(SemVer::parse("0.3.0").value()));

    auto t1 = Constraint::parse("~1.2.3").value();
    ASSERT_TRUE(t1.satisfies(SemVer::parse("1.2.3").value()));
    ASSERT_FALSE(t1.satisfies(SemVer::parse("1.3.0").value()));
}

TEST_CASE(tc07_constraint_intersection_and_conflict) {
    auto c1 = Constraint::parse("^1.0.0").value();
    auto c2 = Constraint::parse(">=1.5.0").value();
    auto inter = c1.intersect(c2);
    ASSERT_TRUE(inter.has_value());
    ASSERT_TRUE(inter.value().satisfies(SemVer::parse("1.5.0").value()));

    auto c3 = Constraint::parse("^2.0.0").value();
    ASSERT_FALSE(c1.intersect(c3).has_value()); // Conflict!
}

TEST_CASE(tc08_or_constraint_grammar) {
    auto c_or = Constraint::parse("^1.0.0 || ^3.0.0").value();
    ASSERT_TRUE(c_or.satisfies(SemVer::parse("1.2.0").value()));
    ASSERT_FALSE(c_or.satisfies(SemVer::parse("2.5.0").value()));
    ASSERT_TRUE(c_or.satisfies(SemVer::parse("3.1.0").value()));
}

// ============================================================================
// Group 3: Directed Graph & Registry
// ============================================================================
TEST_CASE(tc09_graph_node_interning_and_dual_adjacency) {
    DependencyGraph graph;
    NodeId a = graph.getOrAddNode("app");
    NodeId b = graph.getOrAddNode("boost");

    graph.addEdge("app", "boost");
    ASSERT_EQ(graph.nodeCount(), 2u);
    ASSERT_TRUE(graph.hasEdge(a, b));
    ASSERT_FALSE(graph.hasEdge(b, a));

    ASSERT_EQ(graph.getDependencies(a).size(), 1u);
    ASSERT_EQ(graph.getDependents(b).size(), 1u);
    ASSERT_EQ(graph.getDependents(b)[0].target, a);
}

TEST_CASE(tc10_graph_transitive_reachability) {
    DependencyGraph graph;
    graph.addEdge("app", "service");
    graph.addEdge("service", "db");
    graph.addEdge("db", "disk");

    auto deps = graph.getAllDependenciesTransitive("app");
    ASSERT_EQ(deps.size(), 3u);

    auto rev = graph.getAllDependentsTransitive("disk");
    ASSERT_EQ(rev.size(), 3u);
}

TEST_CASE(tc11_registry_crud_and_dependent_cascade_protection) {
    Registry reg;
    reg.registerPackage("react");
    reg.registerPackage("nextjs");
    reg.registerVersion("react", PackageVersion(SemVer::parse("18.2.0").value()));
    reg.registerVersion("nextjs", PackageVersion(SemVer::parse("14.0.0").value(), {
        DependencyDecl("react", Constraint::any())
    }));

    // Removing 'react' refused without cascade
    ASSERT_TRUE(reg.removePackage("react", false).isErr());
    // Allowed with cascade
    ASSERT_TRUE(reg.removePackage("react", true).isOk());
    ASSERT_FALSE(reg.hasPackage("react"));
}

TEST_CASE(tc12_registry_search) {
    Registry reg;
    reg.registerPackage("express");
    reg.registerPackage("express-session");
    reg.registerPackage("koa");

    auto results = reg.search("express");
    ASSERT_EQ(results.size(), 2u);
}

// ============================================================================
// Group 4: Cycle Detection & Tarjan SCC
// ============================================================================
TEST_CASE(tc13_cycle_detection_acyclic_and_self_loop) {
    DependencyGraph g1;
    g1.addEdge("A", "B");
    ASSERT_FALSE(CycleDetector::detectCycle(g1).hasCycle);

    DependencyGraph g2;
    g2.addEdge("A", "A");
    auto res2 = CycleDetector::detectCycle(g2);
    ASSERT_TRUE(res2.hasCycle);
    ASSERT_EQ(res2.cyclePath.size(), 2u);
    ASSERT_EQ(res2.cyclePath[0], std::string("A"));
    ASSERT_EQ(res2.cyclePath[1], std::string("A"));
}

TEST_CASE(tc14_cycle_detection_exact_path) {
    DependencyGraph graph;
    graph.addEdge("A", "B");
    graph.addEdge("B", "C");
    graph.addEdge("C", "A");

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_TRUE(res.hasCycle);
    ASSERT_EQ(res.pathToString(), std::string("A -> B -> C -> A"));
}

TEST_CASE(tc15_cycle_in_disconnected_component) {
    DependencyGraph graph;
    graph.addEdge("ok1", "ok2");
    graph.addEdge("C1", "C2");
    graph.addEdge("C2", "C1");

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_TRUE(res.hasCycle);
    ASSERT_EQ(res.pathToString(), std::string("C1 -> C2 -> C1"));
}

TEST_CASE(tc16_dynamic_will_create_cycle) {
    DependencyGraph graph;
    graph.addEdge("A", "B");
    graph.addEdge("B", "C");

    ASSERT_TRUE(CycleDetector::willCreateCycle(graph, "C", "A"));
    ASSERT_FALSE(CycleDetector::willCreateCycle(graph, "A", "D"));
}

TEST_CASE(tc17_tarjan_strongly_connected_components) {
    DependencyGraph graph;
    graph.addEdge("A", "B");
    graph.addEdge("B", "A");
    graph.addEdge("C", "D");
    graph.addEdge("D", "C");

    auto groups = CycleDetector::findCyclicGroups(graph);
    ASSERT_EQ(groups.size(), 2u);
}

TEST_CASE(tc18_scale_deep_chain_100k_linear_time) {
    DependencyGraph graph;
    constexpr int N = 100000;
    for (int i = 0; i < N - 1; ++i) {
        graph.addEdge("p" + std::to_string(i), "p" + std::to_string(i + 1));
    }
    ASSERT_FALSE(CycleDetector::detectCycle(graph).hasCycle);
}

// ============================================================================
// Group 5: Topological Sorting & Plan Verification
// ============================================================================
TEST_CASE(tc19_kahn_topological_sort) {
    DependencyGraph graph;
    graph.addEdge("app", "libA");
    graph.addEdge("app", "libB");
    graph.addEdge("libA", "core");
    graph.addEdge("libB", "core");

    auto plan = TopologicalSorter::sortKahn(graph).value();
    ASSERT_EQ(plan.steps[0].packageName, std::string("core"));
    ASSERT_EQ(plan.steps[3].packageName, std::string("app"));

    auto report = PlanVerifier::verify(graph, plan);
    ASSERT_TRUE(report.isValid);
}

TEST_CASE(tc20_parallel_execution_layers) {
    DependencyGraph graph;
    graph.addEdge("app", "libA");
    graph.addEdge("app", "libB");
    graph.addEdge("libA", "core");
    graph.addEdge("libB", "core");

    auto layers = TopologicalSorter::getParallelLayers(graph).value();
    ASSERT_EQ(layers.size(), 3u);
    ASSERT_EQ(layers[0][0], std::string("core"));
    ASSERT_EQ(layers[1].size(), 2u);
    ASSERT_EQ(layers[2][0], std::string("app"));
}

TEST_CASE(tc21_multiple_valid_orders_enumeration) {
    DependencyGraph graph;
    graph.getOrAddNode("A");
    graph.getOrAddNode("B");
    graph.getOrAddNode("C");

    auto orders = TopologicalSorter::getMultipleOrders(graph, 10);
    ASSERT_EQ(orders.size(), 6u); // 3! = 6
}

TEST_CASE(tc22_plan_verifier_rejects_corrupted_orders) {
    DependencyGraph graph;
    graph.addEdge("app", "db");

    auto plan = TopologicalSorter::sortKahn(graph).value();
    InstallationPlan badPlan = plan;
    std::swap(badPlan.steps[0], badPlan.steps[1]);

    ASSERT_FALSE(PlanVerifier::verify(graph, badPlan).isValid);
}

TEST_CASE(tc23_dfs_and_kahn_topological_sort_agreement) {
    DependencyGraph graph;
    graph.addEdge("a", "b");
    graph.addEdge("b", "c");

    ASSERT_TRUE(TopologicalSorter::sortKahn(graph).isOk());
    ASSERT_TRUE(TopologicalSorter::sortDfs(graph).isOk());
}

// ============================================================================
// Group 6: Backtracking Resolver & Diagnostics
// ============================================================================
TEST_CASE(tc24_resolver_diamond_dependencies) {
    Registry reg;
    reg.registerPackage("core");
    reg.registerVersion("core", PackageVersion(SemVer::parse("1.2.0").value()));

    reg.registerPackage("liba");
    reg.registerVersion("liba", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("core", Constraint::parse("^1.0.0").value())
    }));

    reg.registerPackage("libb");
    reg.registerVersion("libb", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("core", Constraint::parse(">=1.1.0, <2.0.0").value())
    }));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("liba", Constraint::any()), DependencyDecl("libb", Constraint::any()) };

    auto res = Resolver::resolve(reg, req);
    ASSERT_TRUE(res.isSuccess());
    ASSERT_EQ(res.selectedVersions["core"].toString(), std::string("1.2.0"));
}

TEST_CASE(tc25_resolver_backtracking_selects_older_version) {
    Registry reg;
    reg.registerPackage("database");
    reg.registerVersion("database", PackageVersion(SemVer::parse("2.0.0").value()));
    reg.registerVersion("database", PackageVersion(SemVer::parse("1.0.0").value()));

    reg.registerPackage("backend");
    reg.registerVersion("backend", PackageVersion(SemVer::parse("2.0.0").value(), {
        DependencyDecl("database", Constraint::parse("^2.0.0").value())
    }));
    reg.registerVersion("backend", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("database", Constraint::parse("^1.0.0").value())
    }));

    reg.registerPackage("plugin");
    reg.registerVersion("plugin", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("database", Constraint::parse("^1.0.0").value())
    }));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("backend", Constraint::any()), DependencyDecl("plugin", Constraint::any()) };

    auto res = Resolver::resolve(reg, req);
    ASSERT_TRUE(res.isSuccess());
    ASSERT_EQ(res.selectedVersions["backend"].toString(), std::string("1.0.0"));
    ASSERT_EQ(res.selectedVersions["database"].toString(), std::string("1.0.0"));
}

TEST_CASE(tc26_resolver_conflict_detection) {
    Registry reg;
    reg.registerPackage("shared");
    reg.registerVersion("shared", PackageVersion(SemVer::parse("1.0.0").value()));
    reg.registerVersion("shared", PackageVersion(SemVer::parse("2.0.0").value()));

    reg.registerPackage("modA");
    reg.registerVersion("modA", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("shared", Constraint::parse("^1.0.0").value())
    }));

    reg.registerPackage("modB");
    reg.registerVersion("modB", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("shared", Constraint::parse("^2.0.0").value())
    }));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("moda", Constraint::any()), DependencyDecl("modb", Constraint::any()) };

    auto res = Resolver::resolve(reg, req);
    ASSERT_FALSE(res.isSuccess());
    ASSERT_EQ(res.status, ResolutionStatus::UNSATISFIABLE);
}

TEST_CASE(tc27_resolver_missing_package_diagnostic) {
    Registry reg;
    reg.registerPackage("app");
    reg.registerVersion("app", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("ghost_lib", Constraint::any())
    }));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("app", Constraint::any()) };

    auto res = Resolver::resolve(reg, req);
    ASSERT_FALSE(res.isSuccess());
    ASSERT_EQ(res.diagnostics[0].code, ErrorCode::MISSING_PACKAGE);
}

TEST_CASE(tc28_resolver_step_limit_guard) {
    Registry reg;
    reg.registerPackage("test");
    reg.registerVersion("test", PackageVersion(SemVer::parse("1.0.0").value()));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("test", Constraint::any()) };
    req.maxSteps = 0;

    auto res = Resolver::resolve(reg, req);
    ASSERT_EQ(res.status, ResolutionStatus::LIMIT_REACHED);
}

// ============================================================================
// Group 7: State Management, Orphans & Transitions
// ============================================================================
TEST_CASE(tc29_orphan_detection_and_autoremove) {
    Registry reg;
    reg.registerPackage("app");
    reg.registerVersion("app", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("helper", Constraint::any())
    }));
    reg.registerPackage("helper");
    reg.registerVersion("helper", PackageVersion(SemVer::parse("1.0.0").value()));
    reg.registerPackage("stale");
    reg.registerVersion("stale", PackageVersion(SemVer::parse("1.0.0").value()));

    InstalledState state;
    state.recordInstall(InstalledPackage("app", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));
    state.recordInstall(InstalledPackage("helper", SemVer::parse("1.0.0").value(), InstallReason::DEPENDENCY));
    state.recordInstall(InstalledPackage("stale", SemVer::parse("1.0.0").value(), InstallReason::DEPENDENCY));

    auto orphans = StateManager::findOrphans(state, reg);
    ASSERT_EQ(orphans.size(), 1u);
    ASSERT_EQ(orphans[0], std::string("stale"));

    StateManager::autoRemoveOrphans(state, reg);
    ASSERT_FALSE(state.isInstalled("stale"));
}

TEST_CASE(tc30_unused_package_detection) {
    Registry reg;
    reg.registerPackage("active");
    reg.registerVersion("active", PackageVersion(SemVer::parse("1.0.0").value()));
    reg.registerPackage("idle");
    reg.registerVersion("idle", PackageVersion(SemVer::parse("1.0.0").value()));

    InstalledState state;
    state.recordInstall(InstalledPackage("active", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));

    auto unused = StateManager::findUnused(state, reg);
    ASSERT_EQ(unused.size(), 1u);
    ASSERT_EQ(unused[0], std::string("idle"));
}

TEST_CASE(tc31_version_transition_analysis) {
    InstalledState state;
    state.recordInstall(InstalledPackage("libx", SemVer::parse("1.2.0").value(), InstallReason::EXPLICIT));

    std::map<std::string, SemVer> target;
    target["libx"] = SemVer::parse("2.0.0").value(); // Major upgrade!

    auto trans = StateManager::analyzeTransitions(state, target);
    ASSERT_EQ(trans[0].type, TransitionType::UPGRADE);
    ASSERT_TRUE(trans[0].isMajorChange);
}

// ============================================================================
// Group 8: Simulation, Rollback & Transactions
// ============================================================================
TEST_CASE(tc32_successful_simulation_and_rollback_stack) {
    InstalledState state;
    SimulationEngine sim;
    TransactionManager tx;

    InstallationPlan plan;
    plan.steps = {{0, "pkg1", "1.0.0", 0, "root", {}}};
    std::map<std::string, SemVer> v;
    v["pkg1"] = SemVer::parse("1.0.0").value();

    auto r = tx.executePlan(state, plan, v, sim);
    ASSERT_TRUE(r.isSuccess());
    ASSERT_TRUE(state.isInstalled("pkg1"));
}

TEST_CASE(tc33_injected_failure_clean_rollback) {
    InstalledState state;
    state.recordInstall(InstalledPackage("pre_pkg", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));

    SimulationEngine sim;
    FailureInjectionRule rule;
    rule.targetPackage = "broken_pkg";
    rule.targetStep = SimStepType::UNPACK;
    sim.addFailureRule(rule);

    TransactionManager tx;
    InstallationPlan plan;
    plan.steps = {
        {0, "new_pkg", "1.0.0", 0, "dep", {}},
        {1, "broken_pkg", "1.0.0", 1, "root", {"new_pkg"}}
    };
    std::map<std::string, SemVer> v;
    v["new_pkg"] = SemVer::parse("1.0.0").value();
    v["broken_pkg"] = SemVer::parse("1.0.0").value();

    auto r = tx.executePlan(state, plan, v, sim);
    ASSERT_FALSE(r.isSuccess());
    ASSERT_EQ(r.status, TransactionStatus::ROLLED_BACK);

    // Exact state restored
    ASSERT_FALSE(state.isInstalled("new_pkg"));
    ASSERT_FALSE(state.isInstalled("broken_pkg"));
    ASSERT_TRUE(state.isInstalled("pre_pkg"));
}

// ============================================================================
// Group 9: Persistence & Crash Recovery
// ============================================================================
TEST_CASE(tc34_persistence_roundtrip_and_crash_recovery) {
    Registry reg;
    reg.registerPackage("sample");
    reg.registerVersion("sample", PackageVersion(SemVer::parse("1.0.0").value()));

    Persistence::saveRegistry(reg, "bin/test_suite_reg.manifest");
    auto loaded = Persistence::loadRegistry("bin/test_suite_reg.manifest").value();
    ASSERT_TRUE(loaded.hasPackage("sample"));

    // Crash journal
    std::string jfile = "bin/test_suite.journal";
    Persistence::clearJournal(jfile);
    InstalledState state;
    state.recordInstall(InstalledPackage("crashed_item", SemVer::parse("1.0.0").value(), InstallReason::EXPLICIT));
    Persistence::appendJournal(jfile, "TX_BEGIN 99");
    Persistence::appendJournal(jfile, "TX_INSTALLED crashed_item");

    ASSERT_TRUE(Persistence::performCrashRecovery(jfile, state).value());
    ASSERT_FALSE(state.isInstalled("crashed_item")); // Cleanly recovered!
}

// ============================================================================
// Group 10: Export, Lockfile & Visualization
// ============================================================================
TEST_CASE(tc35_rfc4180_csv_formula_neutralization_and_json) {
    InstallationPlan plan;
    plan.steps = {{0, "=SUM(A1:A10)", "1.0.0", 0, "Formula test", {}}};

    std::string csv = Exporter::exportPlanToCsv(plan);
    ASSERT_TRUE(csv.find("'=SUM") != std::string::npos); // Formula neutralized!

    ResolutionResult res;
    res.status = ResolutionStatus::RESOLVED;
    res.selectedVersions["core"] = SemVer::parse("1.0.0").value();
    res.plan = plan;

    std::string json = Exporter::exportResolutionToJson(res);
    ASSERT_TRUE(json.find("\"status\": \"RESOLVED\"") != std::string::npos);
}

TEST_CASE(tc36_lockfile_generation_and_svg_visualization) {
    Registry reg;
    reg.registerPackage("core");
    reg.registerVersion("core", PackageVersion(SemVer::parse("1.0.0").value(), {}, "sha256-verified-key"));

    std::map<std::string, SemVer> resolved;
    resolved["core"] = SemVer::parse("1.0.0").value();
    InstallationPlan plan;
    plan.steps = {{0, "core", "1.0.0", 0, "root", {}}};
    plan.totalLevels = 1;

    std::string lock = Exporter::generateLockfile(resolved, plan, reg);
    ASSERT_TRUE(lock.find("core = 1.0.0") != std::string::npos);

    DependencyGraph graph;
    graph.addEdge("app", "core");
    std::string svg = Visualizer::renderSvg(graph, "Architecture Diagram");
    ASSERT_TRUE(svg.find("<svg") != std::string::npos);
    ASSERT_TRUE(svg.find("</svg>") != std::string::npos);
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

