#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <stdexcept>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace MiniTest {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class Runner {
public:
    static Runner& instance() {
        static Runner r;
        return r;
    }

    void registerTest(const std::string& name, std::function<void()> func) {
        tests_.push_back({name, func});
    }

    int runAll() {
        enableVirtualTerminal();

        int passed = 0;
        int failed = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        std::cout << "\n=======================================================\n";
        std::cout << "  RUNNING MINI TEST RUNNER (" << tests_.size() << " test cases)\n";
        std::cout << "=======================================================\n";

        for (const auto& test : tests_) {
            std::cout << "[ RUN      ] " << test.name << "\n";
            auto t0 = std::chrono::high_resolution_clock::now();
            try {
                test.func();
                auto t1 = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.0;
                std::cout << "\033[32m[       OK ]\033[0m " << test.name << " (" << ms << " ms)\n";
                passed++;
            } catch (const std::exception& ex) {
                auto t1 = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.0;
                std::cout << "\033[31m[  FAILED  ]\033[0m " << test.name << " (" << ms << " ms)\n";
                std::cout << "    Reason: " << ex.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "\033[31m[  FAILED  ]\033[0m " << test.name << "\n";
                std::cout << "    Reason: Unknown exception thrown\n";
                failed++;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "-------------------------------------------------------\n";
        std::cout << "Total: " << tests_.size() 
                  << " | \033[32mPassed: " << passed << "\033[0m"
                  << " | \033[31mFailed: " << failed << "\033[0m"
                  << " (" << totalMs << " ms)\n";
        std::cout << "=======================================================\n\n";

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> tests_;

    void enableVirtualTerminal() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
    }
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        Runner::instance().registerTest(name, func);
    }
};

} // namespace MiniTest

#define TEST_CASE(name) \
    void name(); \
    static ::MiniTest::TestRegistrar registrar_##name(#name, name); \
    void name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #condition ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (false)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::ostringstream ss; \
            ss << "Assertion failed: NOT (" #condition ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (false)

#define ASSERT_EQ(val1, val2) \
    do { \
        if (!((val1) == (val2))) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #val1 " == " #val2 ") [" << (val1) << " != " << (val2) << "] at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (false)

#define ASSERT_NE(val1, val2) \
    do { \
        if ((val1) == (val2)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #val1 " != " #val2 ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (false)

