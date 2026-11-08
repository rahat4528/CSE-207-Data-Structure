#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <sstream>
#include <cmath>

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

    void addTest(const std::string& name, std::function<void()> func) {
        m_tests.push_back({name, std::move(func)});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;
        auto totalStart = std::chrono::steady_clock::now();

        std::cout << "\n======================================================\n";
        std::cout << " RUNNING MINI-TEST SUITE (" << m_tests.size() << " Tests Registered)\n";
        std::cout << "======================================================\n";

        for (const auto& test : m_tests) {
            std::cout << "[ RUN      ] " << test.name << std::endl;
            auto start = std::chrono::steady_clock::now();
            try {
                test.func();
                auto end = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                std::cout << "[       OK ] " << test.name << " (" << elapsed << " us)" << std::endl;
                passed++;
            } catch (const std::exception& e) {
                auto end = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                std::cout << "[  FAILED  ] " << test.name << " (" << elapsed << " us)" << std::endl;
                std::cout << "             Exception: " << e.what() << std::endl;
                failed++;
            } catch (...) {
                auto end = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                std::cout << "[  FAILED  ] " << test.name << " (" << elapsed << " us)" << std::endl;
                std::cout << "             Unknown exception caught." << std::endl;
                failed++;
            }
        }

        auto totalEnd = std::chrono::steady_clock::now();
        auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

        std::cout << "======================================================\n";
        std::cout << " SUMMARY: " << passed << " PASSED, " << failed << " FAILED in " << totalElapsed << " ms\n";
        std::cout << "======================================================\n\n";

        return (failed == 0) ? 0 : 1;
    }

private:
    std::vector<TestCase> m_tests;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::instance().addTest(name, std::move(func));
    }
};

class AssertionException : public std::exception {
public:
    explicit AssertionException(std::string msg) : m_msg(std::move(msg)) {}
    const char* what() const noexcept override { return m_msg.c_str(); }
private:
    std::string m_msg;
};

} // namespace minitest

#define TEST_CASE(name) \
    static void name(); \
    static ::minitest::TestRegistrar registrar_##name(#name, name); \
    static void name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw ::minitest::AssertionException(oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream oss; \
            oss << "Assertion failed: NOT (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw ::minitest::AssertionException(oss.str()); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: (" #a " == " #b ") [" << (a) << " != " << (b) << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::minitest::AssertionException(oss.str()); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: (" #a " != " #b ") [" << (a) << " == " << (b) << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::minitest::AssertionException(oss.str()); \
        } \
    } while (0)

#define ASSERT_THROWS(expr, ExceptionType) \
    do { \
        bool caught = false; \
        try { \
            expr; \
        } catch (const ExceptionType&) { \
            caught = true; \
        } catch (...) { \
            std::ostringstream oss; \
            oss << "Expected exception " #ExceptionType " but different exception thrown at " << __FILE__ << ":" << __LINE__; \
            throw ::minitest::AssertionException(oss.str()); \
        } \
        if (!caught) { \
            std::ostringstream oss; \
            oss << "Expected exception " #ExceptionType " not thrown at " << __FILE__ << ":" << __LINE__; \
            throw ::minitest::AssertionException(oss.str()); \
        } \
    } while (0)
