#include "TestRunner.h"
#include "DependencyGraph.h"
#include "Registry.h"

using namespace DepResolver;

TEST_CASE(test_node_interning_and_edges) {
    DependencyGraph graph;
    NodeId a = graph.getOrAddNode("app");
    NodeId b = graph.getOrAddNode("boost");
    NodeId c = graph.getOrAddNode("openssl");

    ASSERT_EQ(graph.nodeCount(), 3u);
    ASSERT_EQ(graph.getNodeName(a), std::string("app"));
    ASSERT_EQ(graph.getNodeName(b), std::string("boost"));

    // Add edges: app -> boost, app -> openssl
    auto res1 = graph.addEdge("app", "boost", Constraint::parse("^1.70.0").value());
    ASSERT_TRUE(res1.isOk());

    auto res2 = graph.addEdge("app", "openssl", Constraint::parse("^1.1.1").value());
    ASSERT_TRUE(res2.isOk());

    ASSERT_EQ(graph.edgeCount(), 2u);
    ASSERT_TRUE(graph.hasEdge(a, b));
    ASSERT_TRUE(graph.hasEdge(a, c));
    ASSERT_FALSE(graph.hasEdge(b, a));

    // Check forward adjacency (dependencies of app)
    const auto& fwd = graph.getDependencies(a);
    ASSERT_EQ(fwd.size(), 2u);

    // Check reverse adjacency (dependents of boost: who requires boost?)
    const auto& rev = graph.getDependents(b);
    ASSERT_EQ(rev.size(), 1u);
    ASSERT_EQ(rev[0].target, a); // app requires boost
}

TEST_CASE(test_transitive_reachability) {
    // Chain: app -> service -> database -> storage
    DependencyGraph graph;
    graph.addEdge("app", "service");
    graph.addEdge("service", "database");
    graph.addEdge("database", "storage");

    auto deps = graph.getAllDependenciesTransitive("app");
    ASSERT_EQ(deps.size(), 3u);
    ASSERT_EQ(deps[0], std::string("database"));
    ASSERT_EQ(deps[1], std::string("service"));
    ASSERT_EQ(deps[2], std::string("storage"));

    // Reverse reachability: who transitively depends on storage?
    auto rev = graph.getAllDependentsTransitive("storage");
    ASSERT_EQ(rev.size(), 3u);
    ASSERT_EQ(rev[0], std::string("app"));
    ASSERT_EQ(rev[1], std::string("database"));
    ASSERT_EQ(rev[2], std::string("service"));
}

TEST_CASE(test_registry_crud_and_dependent_protection) {
    Registry reg;
    ASSERT_TRUE(reg.registerPackage("react", "UI Library").isOk());
    ASSERT_TRUE(reg.registerPackage("nextjs", "Fullstack Framework").isOk());

    // Duplicate package registration must be rejected
    ASSERT_TRUE(reg.registerPackage("react").isErr());

    // Add versions
    PackageVersion vReact(SemVer::parse("18.2.0").value());
    ASSERT_TRUE(reg.registerVersion("react", vReact).isOk());

    // Duplicate version must be rejected
    ASSERT_TRUE(reg.registerVersion("react", vReact).isErr());

    // NextJS depends on React
    PackageVersion vNext(SemVer::parse("14.0.0").value(), {
        DependencyDecl("react", Constraint::parse("^18.0.0").value())
    });
    ASSERT_TRUE(reg.registerVersion("nextjs", vNext).isOk());

    // Removing 'react' directly without cascade must be refused!
    auto remRes = reg.removePackage("react", false);
    ASSERT_TRUE(remRes.isErr());
    ASSERT_EQ(remRes.error().code, ErrorCode::DEPENDENT_EXISTS);

    // Removing with cascade should succeed
    auto remCascade = reg.removePackage("react", true);
    ASSERT_TRUE(remCascade.isOk());
    ASSERT_FALSE(reg.hasPackage("react"));
}

TEST_CASE(test_registry_search) {
    Registry reg;
    reg.registerPackage("express", "Fast node web framework");
    reg.registerPackage("express-session", "Session middleware");
    reg.registerPackage("fastify", "Fast low overhead web framework");

    auto results = reg.search("express");
    ASSERT_EQ(results.size(), 2u);
    ASSERT_EQ(results[0]->name(), std::string("express"));
    ASSERT_EQ(results[1]->name(), std::string("express-session"));
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

