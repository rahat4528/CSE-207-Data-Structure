#include "TestRunner.h"
#include "Resolver.h"

using namespace DepResolver;

TEST_CASE(test_diamond_dependency_resolution) {
    Registry reg;
    // Common library 'logger'
    reg.registerPackage("logger");
    reg.registerVersion("logger", PackageVersion(SemVer::parse("1.2.0").value()));
    reg.registerVersion("logger", PackageVersion(SemVer::parse("1.0.0").value()));

    // libA requires logger ^1.0.0
    reg.registerPackage("liba");
    reg.registerVersion("liba", PackageVersion(SemVer::parse("2.0.0").value(), {
        DependencyDecl("logger", Constraint::parse("^1.0.0").value())
    }));

    // libB requires logger >=1.1.0, <2.0.0
    reg.registerPackage("libb");
    reg.registerVersion("libb", PackageVersion(SemVer::parse("3.1.0").value(), {
        DependencyDecl("logger", Constraint::parse(">=1.1.0, <2.0.0").value())
    }));

    // Root app requires liba and libb
    ResolutionRequest req;
    req.rootRequirements = {
        DependencyDecl("liba", Constraint::any()),
        DependencyDecl("libb", Constraint::any())
    };

    auto res = Resolver::resolve(reg, req);
    ASSERT_TRUE(res.isSuccess());
    ASSERT_EQ(res.status, ResolutionStatus::RESOLVED);

    // Should choose logger 1.2.0 (newest compatible version)
    ASSERT_EQ(res.selectedVersions["logger"].toString(), std::string("1.2.0"));
    ASSERT_EQ(res.selectedVersions["liba"].toString(), std::string("2.0.0"));
    ASSERT_EQ(res.selectedVersions["libb"].toString(), std::string("3.1.0"));
}

TEST_CASE(test_backtracking_selects_older_working_version) {
    Registry reg;
    // 'database': v2.0.0 and v1.0.0
    reg.registerPackage("database");
    reg.registerVersion("database", PackageVersion(SemVer::parse("2.0.0").value()));
    reg.registerVersion("database", PackageVersion(SemVer::parse("1.0.0").value()));

    // 'backend':
    // v2.0.0 needs database ^2.0.0
    // v1.0.0 needs database ^1.0.0
    reg.registerPackage("backend");
    reg.registerVersion("backend", PackageVersion(SemVer::parse("2.0.0").value(), {
        DependencyDecl("database", Constraint::parse("^2.0.0").value())
    }));
    reg.registerVersion("backend", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("database", Constraint::parse("^1.0.0").value())
    }));

    // 'legacy_plugin': strictly needs database ^1.0.0
    reg.registerPackage("legacy_plugin");
    reg.registerVersion("legacy_plugin", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("database", Constraint::parse("^1.0.0").value())
    }));

    // User requests backend and legacy_plugin
    // Initially, solver tries backend 2.0.0 -> database 2.0.0 -> legacy_plugin fails (needs database ^1.0.0)!
    // Solver must BACKTRACK to backend 1.0.0 and successfully resolve database 1.0.0!
    ResolutionRequest req;
    req.rootRequirements = {
        DependencyDecl("backend", Constraint::any()),
        DependencyDecl("legacy_plugin", Constraint::any())
    };

    auto res = Resolver::resolve(reg, req);
    ASSERT_TRUE(res.isSuccess());
    ASSERT_EQ(res.selectedVersions["backend"].toString(), std::string("1.0.0"));
    ASSERT_EQ(res.selectedVersions["database"].toString(), std::string("1.0.0"));
}

TEST_CASE(test_version_conflict_detection) {
    Registry reg;
    reg.registerPackage("core");
    reg.registerVersion("core", PackageVersion(SemVer::parse("1.0.0").value()));
    reg.registerVersion("core", PackageVersion(SemVer::parse("2.0.0").value()));

    reg.registerPackage("serviceA");
    reg.registerVersion("serviceA", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("core", Constraint::parse("^1.0.0").value())
    }));

    reg.registerPackage("serviceB");
    reg.registerVersion("serviceB", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("core", Constraint::parse("^2.0.0").value())
    }));

    // Conflicting request: serviceA requires core ^1.0, serviceB requires core ^2.0
    ResolutionRequest req;
    req.rootRequirements = {
        DependencyDecl("servicea", Constraint::any()),
        DependencyDecl("serviceb", Constraint::any())
    };

    auto res = Resolver::resolve(reg, req);
    ASSERT_FALSE(res.isSuccess());
    ASSERT_EQ(res.status, ResolutionStatus::UNSATISFIABLE);

    // Diagnostics must record VERSION_CONFLICT
    bool foundConflict = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.code == ErrorCode::VERSION_CONFLICT) {
            foundConflict = true;
            break;
        }
    }
    ASSERT_TRUE(foundConflict);
}

TEST_CASE(test_missing_package_diagnostic) {
    Registry reg;
    reg.registerPackage("app");
    reg.registerVersion("app", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("non_existent_dep", Constraint::any())
    }));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("app", Constraint::any()) };

    auto res = Resolver::resolve(reg, req);
    ASSERT_FALSE(res.isSuccess());
    ASSERT_EQ(res.status, ResolutionStatus::UNSATISFIABLE);

    bool foundMissing = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.code == ErrorCode::MISSING_PACKAGE) {
            foundMissing = true;
            ASSERT_EQ(diag.packageName, std::string("non_existent_dep"));
            break;
        }
    }
    ASSERT_TRUE(foundMissing);
}

TEST_CASE(test_circular_dependency_in_resolution) {
    Registry reg;
    // Circular registry: pkg1 -> pkg2 -> pkg1
    reg.registerPackage("pkg1");
    reg.registerVersion("pkg1", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("pkg2", Constraint::any())
    }));

    reg.registerPackage("pkg2");
    reg.registerVersion("pkg2", PackageVersion(SemVer::parse("1.0.0").value(), {
        DependencyDecl("pkg1", Constraint::any())
    }));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("pkg1", Constraint::any()) };

    auto res = Resolver::resolve(reg, req);
    ASSERT_FALSE(res.isSuccess());
    ASSERT_EQ(res.status, ResolutionStatus::UNSATISFIABLE);

    bool foundCycle = false;
    for (const auto& diag : res.diagnostics) {
        if (diag.code == ErrorCode::CYCLE_DETECTED) {
            foundCycle = true;
            break;
        }
    }
    ASSERT_TRUE(foundCycle);
}

TEST_CASE(test_step_limit_reached) {
    Registry reg;
    reg.registerPackage("dummy");
    reg.registerVersion("dummy", PackageVersion(SemVer::parse("1.0.0").value()));

    ResolutionRequest req;
    req.rootRequirements = { DependencyDecl("dummy", Constraint::any()) };
    req.maxSteps = 0; // Immediate limit reached!

    auto res = Resolver::resolve(reg, req);
    ASSERT_FALSE(res.isSuccess());
    ASSERT_EQ(res.status, ResolutionStatus::LIMIT_REACHED);
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

