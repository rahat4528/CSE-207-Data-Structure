#include "TestRunner.h"
#include "SemVer.h"
#include "Constraint.h"

using namespace DepResolver;

TEST_CASE(test_semver_precedence_chain) {
    // Official SemVer 2.0.0 required precedence test chain:
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
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

TEST_CASE(test_build_metadata_ignored_in_precedence) {
    auto b1 = SemVer::parse("1.0.0+20130313144700").value();
    auto b2 = SemVer::parse("1.0.0+exp.sha.5114f85").value();
    ASSERT_TRUE(b1 == b2);
    ASSERT_FALSE(b1 < b2);
    ASSERT_FALSE(b1 > b2);
}

TEST_CASE(test_strict_parsing_rejects_leading_zeros) {
    ASSERT_TRUE(SemVer::parse("01.0.0").isErr());
    ASSERT_TRUE(SemVer::parse("1.02.0").isErr());
    ASSERT_TRUE(SemVer::parse("1.0.03").isErr());
    ASSERT_TRUE(SemVer::parse("1.0.0-01").isErr()); // Numeric prerelease with leading zero
    ASSERT_TRUE(SemVer::parse("0.0.0").isOk());      // Single zero is valid
}

TEST_CASE(test_caret_constraint_semantics) {
    auto c1 = Constraint::parse("^1.2.3").value();
    ASSERT_TRUE(c1.satisfies(SemVer::parse("1.2.3").value()));
    ASSERT_TRUE(c1.satisfies(SemVer::parse("1.9.9").value()));
    ASSERT_FALSE(c1.satisfies(SemVer::parse("2.0.0").value()));
    ASSERT_FALSE(c1.satisfies(SemVer::parse("1.2.2").value()));

    // Caret with 0.x.x
    auto c0 = Constraint::parse("^0.2.3").value();
    ASSERT_TRUE(c0.satisfies(SemVer::parse("0.2.3").value()));
    ASSERT_TRUE(c0.satisfies(SemVer::parse("0.2.9").value()));
    ASSERT_FALSE(c0.satisfies(SemVer::parse("0.3.0").value()));

    // Caret with 0.0.x (exact patch)
    auto c00 = Constraint::parse("^0.0.3").value();
    ASSERT_TRUE(c00.satisfies(SemVer::parse("0.0.3").value()));
    ASSERT_FALSE(c00.satisfies(SemVer::parse("0.0.4").value()));
}

TEST_CASE(test_tilde_constraint_semantics) {
    auto t1 = Constraint::parse("~1.2.3").value();
    ASSERT_TRUE(t1.satisfies(SemVer::parse("1.2.3").value()));
    ASSERT_TRUE(t1.satisfies(SemVer::parse("1.2.9").value()));
    ASSERT_FALSE(t1.satisfies(SemVer::parse("1.3.0").value()));
}

TEST_CASE(test_constraint_intersection_and_conflict) {
    auto c1 = Constraint::parse("^1.0.0").value();
    auto c2 = Constraint::parse(">=1.5.0").value();

    // Intersection compatible: [1.5.0, 2.0.0)
    auto inter = c1.intersect(c2);
    ASSERT_TRUE(inter.has_value());
    ASSERT_TRUE(inter.value().satisfies(SemVer::parse("1.5.0").value()));
    ASSERT_TRUE(inter.value().satisfies(SemVer::parse("1.9.0").value()));
    ASSERT_FALSE(inter.value().satisfies(SemVer::parse("1.4.9").value()));
    ASSERT_FALSE(inter.value().satisfies(SemVer::parse("2.0.0").value()));

    // Conflict: ^1.0.0 vs ^2.0.0 (Disjoint intervals)
    auto c3 = Constraint::parse("^2.0.0").value();
    auto conflict = c1.intersect(c3);
    ASSERT_FALSE(conflict.has_value()); // Conflict correctly identified!
}

TEST_CASE(test_or_constraint_semantics) {
    auto c_or = Constraint::parse("^1.0.0 || ^3.0.0").value();
    ASSERT_TRUE(c_or.satisfies(SemVer::parse("1.2.0").value()));
    ASSERT_FALSE(c_or.satisfies(SemVer::parse("2.5.0").value()));
    ASSERT_TRUE(c_or.satisfies(SemVer::parse("3.1.0").value()));
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

