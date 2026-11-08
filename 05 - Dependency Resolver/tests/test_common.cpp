#include "TestRunner.h"
#include "Common.h"

using namespace DepResolver;

TEST_CASE(test_error_codes) {
    ASSERT_EQ(errorCodeToString(ErrorCode::SUCCESS), std::string("SUCCESS"));
    ASSERT_EQ(errorCodeToString(ErrorCode::CYCLE_DETECTED), std::string("CYCLE_DETECTED"));
    ASSERT_EQ(errorCodeToString(ErrorCode::VERSION_CONFLICT), std::string("VERSION_CONFLICT"));
}

TEST_CASE(test_diagnostic_chain) {
    Diagnostic diag(ErrorCode::VERSION_CONFLICT, "libssl", "Version mismatch", {"web_app", "database", "libssl"});
    ASSERT_TRUE(diag.isError());
    ASSERT_EQ(diag.chainToString(), std::string("web_app -> database -> libssl"));
}

TEST_CASE(test_package_name_validation) {
    // Valid names
    auto r1 = Validator::validatePackageName("boost");
    ASSERT_TRUE(r1.isOk());
    ASSERT_EQ(r1.value(), std::string("boost"));

    auto r2 = Validator::validatePackageName("My_Lib-2.0");
    ASSERT_TRUE(r2.isOk());
    ASSERT_EQ(r2.value(), std::string("my_lib-2.0")); // normalized to lowercase

    // Invalid names
    auto r3 = Validator::validatePackageName("");
    ASSERT_TRUE(r3.isErr());
    ASSERT_EQ(r3.error().code, ErrorCode::INVALID_PACKAGE_NAME);

    auto r4 = Validator::validatePackageName("-invalid_start");
    ASSERT_TRUE(r4.isErr());

    auto r5 = Validator::validatePackageName("invalid/path");
    ASSERT_TRUE(r5.isErr());

    auto r6 = Validator::validatePackageName("invalid$char");
    ASSERT_TRUE(r6.isErr());
}

TEST_CASE(test_text_sanitization) {
    std::string dirty = "SafeText\x1b[31mRed\x07Alert";
    std::string clean = Validator::sanitizeText(dirty);
    ASSERT_FALSE(clean.find('\x07') != std::string::npos);
}

int main() {
    return MiniTest::Runner::instance().runAll();
}

