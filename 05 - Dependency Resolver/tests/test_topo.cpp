#include "TestRunner.h"
#include "TopologicalSorter.h"
#include "PlanVerifier.h"

using namespace DepResolver;

TEST_CASE(test_kahn_topological_sort) {
    DependencyGraph graph;
    // Diamond dependency:
    // app depends on libA and libB
    // libA depends on core
    // libB depends on core
    graph.addEdge("app", "libA");
    graph.addEdge("app", "libB");
    graph.addEdge("libA", "core");
    graph.addEdge("libB", "core");

    auto planRes = TopologicalSorter::sortKahn(graph);
    ASSERT_TRUE(planRes.isOk());

    const auto& plan = planRes.value();
    ASSERT_EQ(plan.steps.size(), 4u);

    // Verify independent plan verifier confirms correctness
    auto report = PlanVerifier::verify(graph, plan);
    ASSERT_TRUE(report.isValid);

    // Check that core (leaf prerequisite) is step 0!
    ASSERT_EQ(plan.steps[0].packageName, std::string("core"));
    // app must be the last step
    ASSERT_EQ(plan.steps[3].packageName, std::string("app"));
}

TEST_CASE(test_parallel_layers) {
    DependencyGraph graph;
    // Layer 0: core
    // Layer 1: libA, libB (independent of each other)
    // Layer 2: app
    graph.addEdge("app", "libA");
    graph.addEdge("app", "libB");
    graph.addEdge("libA", "core");
    graph.addEdge("libB", "core");

    auto layersRes = TopologicalSorter::getParallelLayers(graph);
    ASSERT_TRUE(layersRes.isOk());

    const auto& layers = layersRes.value();
    ASSERT_EQ(layers.size(), 3u);

    ASSERT_EQ(layers[0].size(), 1u);
    ASSERT_EQ(layers[0][0], std::string("core"));

    ASSERT_EQ(layers[1].size(), 2u);
    ASSERT_EQ(layers[1][0], std::string("libA"));
    ASSERT_EQ(layers[1][1], std::string("libB"));

    ASSERT_EQ(layers[2].size(), 1u);
    ASSERT_EQ(layers[2][0], std::string("app"));
}

TEST_CASE(test_multiple_valid_orders) {
    DependencyGraph graph;
    // Disconnected independent packages: A, B, C can be installed in 3! = 6 orders
    graph.getOrAddNode("A");
    graph.getOrAddNode("B");
    graph.getOrAddNode("C");

    auto orders = TopologicalSorter::getMultipleOrders(graph, 10);
    ASSERT_EQ(orders.size(), 6u); // Exactly 3! = 6 valid orders
}

TEST_CASE(test_plan_verifier_rejects_corrupted_plan) {
    DependencyGraph graph;
    graph.addEdge("app", "database");

    // Valid plan
    auto plan = TopologicalSorter::sortKahn(graph).value();
    ASSERT_TRUE(PlanVerifier::verify(graph, plan).isValid);

    // Corrupted plan 1: Reversed order (app before database)
    InstallationPlan badPlan1 = plan;
    std::swap(badPlan1.steps[0], badPlan1.steps[1]);
    auto report1 = PlanVerifier::verify(graph, badPlan1);
    ASSERT_FALSE(report1.isValid);

    // Corrupted plan 2: Duplicate entry
    InstallationPlan badPlan2 = plan;
    badPlan2.steps.push_back(badPlan2.steps[0]);
    auto report2 = PlanVerifier::verify(graph, badPlan2);
    ASSERT_FALSE(report2.isValid);
}

TEST_CASE(test_dfs_and_kahn_agreement_on_acyclic_and_cyclic) {
    DependencyGraph validGraph;
    validGraph.addEdge("frontend", "backend");
    validGraph.addEdge("backend", "db");

    auto kahnRes = TopologicalSorter::sortKahn(validGraph);
    auto dfsRes = TopologicalSorter::sortDfs(validGraph);

    ASSERT_TRUE(kahnRes.isOk());
    ASSERT_TRUE(dfsRes.isOk());

    // Both should agree on cyclic graphs
    DependencyGraph cyclicGraph;
    cyclicGraph.addEdge("X", "Y");
    cyclicGraph.addEdge("Y", "X");

    ASSERT_TRUE(TopologicalSorter::sortKahn(cyclicGraph).isErr());
    ASSERT_TRUE(TopologicalSorter::sortDfs(cyclicGraph).isErr());
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

