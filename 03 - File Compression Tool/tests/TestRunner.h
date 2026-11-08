#ifndef HUFFMAN_TEST_RUNNER_H
#define HUFFMAN_TEST_RUNNER_H

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <functional>
#include <exception>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

namespace MiniTest {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& getRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct AutoRegister {
    AutoRegister(const std::string& name, std::function<void()> func) {
        getRegistry().push_back({name, func});
    }
};

#define TEST_CASE(name) \
    void name(); \
    static ::MiniTest::AutoRegister reg_##name(#name, name); \
    void name()

class AssertionException : public std::exception {
    std::string msg;
public:
    explicit AssertionException(std::string m) : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }
};

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream ss; \
            ss << "Assertion failed: NOT (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        auto valA = (a); \
        auto valB = (b); \
        if (!(valA == valB)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #a " == " #b ") Expected [" << valB << "] but got [" << valA << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        auto valA = (a); \
        auto valB = (b); \
        if (valA == valB) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #a " != " #b ") Got [" << valA << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_LT(a, b) \
    do { \
        auto valA = (a); \
        auto valB = (b); \
        if (!(valA < valB)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #a " < " #b ") [" << valA << " < " << valB << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_LE(a, b) \
    do { \
        auto valA = (a); \
        auto valB = (b); \
        if (!(valA <= valB)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #a " <= " #b ") [" << valA << " <= " << valB << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_GT(a, b) \
    do { \
        auto valA = (a); \
        auto valB = (b); \
        if (!(valA > valB)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #a " > " #b ") [" << valA << " > " << valB << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

#define ASSERT_GE(a, b) \
    do { \
        auto valA = (a); \
        auto valB = (b); \
        if (!(valA >= valB)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #a " >= " #b ") [" << valA << " >= " << valB << "] at " << __FILE__ << ":" << __LINE__; \
            throw ::MiniTest::AssertionException(ss.str()); \
        } \
    } while (0)

inline int runAllTests() {
    auto& tests = getRegistry();
    int passed = 0;
    int failed = 0;

    std::cout << "=======================================================\n";
    std::cout << "  RUNNING HUFFMAN COMPRESSION TEST RUNNER (" << tests.size() << " test cases)\n";
    std::cout << "=======================================================\n";

    auto startTotal = std::chrono::high_resolution_clock::now();

    for (const auto& test : tests) {
        std::cout << "[ RUN      ] " << test.name << "\n";
        auto start = std::chrono::high_resolution_clock::now();
        try {
            test.func();
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "[       OK ] " << test.name << " (" << ms << " ms)\n";
            passed++;
        } catch (const AssertionException& e) {
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "[  FAILED  ] " << test.name << " (" << ms << " ms)\n";
            std::cout << "  --> " << e.what() << "\n";
            failed++;
        } catch (const std::exception& e) {
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "[  FAILED  ] " << test.name << " (Exception: " << e.what() << ") (" << ms << " ms)\n";
            failed++;
        } catch (...) {
            std::cout << "[  FAILED  ] " << test.name << " (Unknown Exception)\n";
            failed++;
        }
    }

    auto endTotal = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(endTotal - startTotal).count();

    std::cout << "-------------------------------------------------------\n";
    std::cout << "Total: " << tests.size() << " | Passed: " << passed << " | Failed: " << failed
              << " (" << totalMs << " ms)\n";
    std::cout << "=======================================================\n\n";

    return (failed == 0) ? 0 : 1;
}

} // namespace MiniTest

#endif // HUFFMAN_TEST_RUNNER_H
