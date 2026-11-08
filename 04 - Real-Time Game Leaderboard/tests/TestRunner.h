#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <cmath>
#include <sstream>

namespace MiniTest {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void registerTest(const std::string& name, std::function<void()> func) {
        tests_.push_back({name, std::move(func)});
    }

    const std::vector<TestCase>& tests() const {
        return tests_;
    }

    void clear() {
        tests_.clear();
    }

private:
    std::vector<TestCase> tests_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::instance().registerTest(name, std::move(func));
    }
};

#define TEST_CASE(name) \
    void name(); \
    static MiniTest::TestRegistrar registrar_##name(#name, name); \
    void name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream oss; \
            oss << "Assertion failed: NOT (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_EQ(val1, val2) \
    do { \
        if (!((val1) == (val2))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #val1 " == " #val2 " (Expected [" << (val2) << "] but got [" << (val1) << "]) at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_NE(val1, val2) \
    do { \
        if ((val1) == (val2)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #val1 " != " #val2 " (Both were [" << (val1) << "]) at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_LT(val1, val2) \
    do { \
        if (!((val1) < (val2))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #val1 " < " #val2 " (" << (val1) << " >= " << (val2) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_LE(val1, val2) \
    do { \
        if (!((val1) <= (val2))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #val1 " <= " #val2 " (" << (val1) << " > " << (val2) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_GT(val1, val2) \
    do { \
        if (!((val1) > (val2))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #val1 " > " #val2 " (" << (val1) << " <= " << (val2) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_GE(val1, val2) \
    do { \
        if (!((val1) >= (val2))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #val1 " >= " #val2 " (" << (val1) << " < " << (val2) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

#define ASSERT_NEAR(val1, val2, eps) \
    do { \
        if (std::abs((val1) - (val2)) > (eps)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: |" #val1 " - " #val2 "| <= " #eps " (Diff: " << std::abs((val1) - (val2)) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(oss.str()); \
        } \
    } while (0)

inline int runAllTests() {
    const auto& tests = TestRegistry::instance().tests();
    int passed = 0;
    int failed = 0;

    std::cout << "\n=======================================================\n";
    std::cout << "  RUNNING MINI TEST RUNNER (" << tests.size() << " test cases)\n";
    std::cout << "=======================================================\n";

    auto totalStart = std::chrono::high_resolution_clock::now();

    for (const auto& test : tests) {
        std::cout << "[ RUN      ] " << test.name << "\n";
        auto start = std::chrono::high_resolution_clock::now();
        try {
            test.func();
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "[       OK ] " << test.name << " (" << ms << " ms)\n";
            passed++;
        } catch (const std::exception& ex) {
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "[  FAILED  ] " << test.name << " (" << ms << " ms)\n";
            std::cout << "             " << ex.what() << "\n";
            failed++;
        } catch (...) {
            std::cout << "[  FAILED  ] " << test.name << " (Unknown exception)\n";
            failed++;
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    std::cout << "-------------------------------------------------------\n";
    std::cout << "Total: " << tests.size() << " | Passed: " << passed << " | Failed: " << failed
              << " (" << totalMs << " ms)\n";
    std::cout << "=======================================================\n\n";

    return (failed == 0) ? 0 : 1;
}

} // namespace MiniTest

#endif // TEST_RUNNER_H
