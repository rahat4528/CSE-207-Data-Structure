#ifndef CRAWLER_MINITEST_H
#define CRAWLER_MINITEST_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <sstream>

namespace minitest {

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
        m_tests.push_back({name, func});
    }

    int runAll() {
        std::cout << "\n======================================================\n";
        std::cout << " RUNNING CRAWLER TEST SUITE (" << m_tests.size() << " Tests Registered)\n";
        std::cout << "======================================================\n";

        size_t passed = 0;
        size_t failed = 0;
        auto suiteStart = std::chrono::high_resolution_clock::now();

        for (const auto& test : m_tests) {
            std::cout << "[ RUN      ] " << test.name << "\n";
            auto start = std::chrono::high_resolution_clock::now();
            bool testPassed = true;
            try {
                test.func();
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.name << ": " << e.what() << "\n";
                testPassed = false;
            } catch (...) {
                std::cout << "[  FAILED  ] " << test.name << ": Unknown exception\n";
                testPassed = false;
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

            if (testPassed) {
                std::cout << "[       OK ] " << test.name << " (" << durationUs << " us)\n";
                passed++;
            } else {
                failed++;
            }
        }

        auto suiteEnd = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(suiteEnd - suiteStart).count();

        std::cout << "======================================================\n";
        std::cout << " SUMMARY: " << passed << " PASSED, " << failed << " FAILED in "
                  << totalMs << " ms\n";
        std::cout << "======================================================\n\n";

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> m_tests;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::instance().registerTest(name, func);
    }
};

} // namespace minitest

#define TEST_CASE(name) \
    void name(); \
    static ::minitest::TestRegistrar registrar_##name(#name, name); \
    void name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(_oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: NOT (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(_oss.str()); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: (" #a " == " #b ") [" << (a) << " != " << (b) << "] at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(_oss.str()); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: (" #a " != " #b ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(_oss.str()); \
        } \
    } while (0)

#endif // CRAWLER_MINITEST_H

