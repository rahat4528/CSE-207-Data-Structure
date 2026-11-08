#include "TestRunner.h"
#include "CycleDetector.h"

using namespace DepResolver;

TEST_CASE(test_acyclic_graph) {
    DependencyGraph graph;
    graph.addEdge("A", "B");
    graph.addEdge("B", "C");
    graph.addEdge("A", "C");

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_FALSE(res.hasCycle);
    ASSERT_TRUE(res.cyclePath.empty());
}

TEST_CASE(test_self_loop) {
    DependencyGraph graph;
    graph.addEdge("A", "A");

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_TRUE(res.hasCycle);
    ASSERT_EQ(res.cyclePath.size(), 2u);
    ASSERT_EQ(res.cyclePath[0], std::string("A"));
    ASSERT_EQ(res.cyclePath[1], std::string("A"));
}

TEST_CASE(test_simple_cycle_and_path_output) {
    DependencyGraph graph;
    // A -> B -> C -> A
    graph.addEdge("A", "B");
    graph.addEdge("B", "C");
    graph.addEdge("C", "A");

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_TRUE(res.hasCycle);
    ASSERT_EQ(res.pathToString(), std::string("A -> B -> C -> A"));
}

TEST_CASE(test_cycle_in_disconnected_component) {
    DependencyGraph graph;
    // Component 1: acyclic
    graph.addEdge("root", "dep1");
    graph.addEdge("dep1", "dep2");

    // Component 2: cyclic (X -> Y -> X)
    graph.addEdge("X", "Y");
    graph.addEdge("Y", "X");

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_TRUE(res.hasCycle);
    ASSERT_EQ(res.pathToString(), std::string("X -> Y -> X"));
}

TEST_CASE(test_dynamic_will_create_cycle) {
    DependencyGraph graph;
    graph.addEdge("web", "auth");
    graph.addEdge("auth", "db");

    // Adding db -> web would create cycle
    ASSERT_TRUE(CycleDetector::willCreateCycle(graph, "db", "web"));
    // Adding web -> helper does not create cycle
    ASSERT_FALSE(CycleDetector::willCreateCycle(graph, "web", "helper"));
}

TEST_CASE(test_tarjan_cyclic_groups) {
    DependencyGraph graph;
    // Group 1: A <-> B
    graph.addEdge("A", "B");
    graph.addEdge("B", "A");

    // Group 2: C -> D -> E -> C
    graph.addEdge("C", "D");
    graph.addEdge("D", "E");
    graph.addEdge("E", "C");

    // Acyclic bridge
    graph.addEdge("B", "C");

    auto groups = CycleDetector::findCyclicGroups(graph);
    ASSERT_EQ(groups.size(), 2u);
}

TEST_CASE(test_deep_chain_100k_no_stack_overflow) {
    DependencyGraph graph;
    constexpr int N = 100000;
    // Chain: p0 -> p1 -> p2 ... -> p99999
    for (int i = 0; i < N - 1; ++i) {
        graph.addEdge("p" + std::to_string(i), "p" + std::to_string(i + 1));
    }

    auto res = CycleDetector::detectCycle(graph);
    ASSERT_FALSE(res.hasCycle);
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

