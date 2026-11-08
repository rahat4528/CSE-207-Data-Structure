#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <utility>
#include <cstdint>
#include <iostream>

namespace DepResolver {

// ============================================================================
// Stable Diagnostic Error Codes
// ============================================================================
enum class ErrorCode {
    SUCCESS = 0,
    MISSING_PACKAGE,
    NO_MATCHING_VERSION,
    VERSION_CONFLICT,
    CYCLE_DETECTED,
    UNSATISFIABLE,
    LIMIT_REACHED,
    INVALID_CONSTRAINT,
    INVALID_VERSION_FORMAT,
    INVALID_PACKAGE_NAME,
    ALREADY_EXISTS,
    PACKAGE_NOT_FOUND,
    VERSION_NOT_FOUND,
    DEPENDENCY_NOT_FOUND,
    DEPENDENT_EXISTS,
    PLAN_VERIFICATION_FAILED,
    SIMULATION_FAILED,
    ROLLBACK_FAILED,
    ROLLBACK_INCOMPLETE,
    TRANSACTION_ABORTED,
    JOURNAL_CORRUPT,
    IO_ERROR,
    PARSE_ERROR
};

inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::MISSING_PACKAGE: return "MISSING_PACKAGE";
        case ErrorCode::NO_MATCHING_VERSION: return "NO_MATCHING_VERSION";
        case ErrorCode::VERSION_CONFLICT: return "VERSION_CONFLICT";
        case ErrorCode::CYCLE_DETECTED: return "CYCLE_DETECTED";
        case ErrorCode::UNSATISFIABLE: return "UNSATISFIABLE";
        case ErrorCode::LIMIT_REACHED: return "LIMIT_REACHED";
        case ErrorCode::INVALID_CONSTRAINT: return "INVALID_CONSTRAINT";
        case ErrorCode::INVALID_VERSION_FORMAT: return "INVALID_VERSION_FORMAT";
        case ErrorCode::INVALID_PACKAGE_NAME: return "INVALID_PACKAGE_NAME";
        case ErrorCode::ALREADY_EXISTS: return "ALREADY_EXISTS";
        case ErrorCode::PACKAGE_NOT_FOUND: return "PACKAGE_NOT_FOUND";
        case ErrorCode::VERSION_NOT_FOUND: return "VERSION_NOT_FOUND";
        case ErrorCode::DEPENDENCY_NOT_FOUND: return "DEPENDENCY_NOT_FOUND";
        case ErrorCode::DEPENDENT_EXISTS: return "DEPENDENT_EXISTS";
        case ErrorCode::PLAN_VERIFICATION_FAILED: return "PLAN_VERIFICATION_FAILED";
        case ErrorCode::SIMULATION_FAILED: return "SIMULATION_FAILED";
        case ErrorCode::ROLLBACK_FAILED: return "ROLLBACK_FAILED";
        case ErrorCode::ROLLBACK_INCOMPLETE: return "ROLLBACK_INCOMPLETE";
        case ErrorCode::TRANSACTION_ABORTED: return "TRANSACTION_ABORTED";
        case ErrorCode::JOURNAL_CORRUPT: return "JOURNAL_CORRUPT";
        case ErrorCode::IO_ERROR: return "IO_ERROR";
        case ErrorCode::PARSE_ERROR: return "PARSE_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

inline std::ostream& operator<<(std::ostream& os, ErrorCode code) {
    return os << errorCodeToString(code);
}

// ============================================================================
// Diagnostic Object with Exact Causality & Dependency Chains
// ============================================================================
struct Diagnostic {
    ErrorCode code{ErrorCode::SUCCESS};
    std::string packageName;
    std::string message;
    std::vector<std::string> dependencyChain; // e.g. ["app", "libA", "libB"]
    std::string detail;

    Diagnostic() = default;
    Diagnostic(ErrorCode c, std::string pkg, std::string msg, 
               std::vector<std::string> chain = {}, std::string det = "")
        : code(c), packageName(std::move(pkg)), message(std::move(msg)),
          dependencyChain(std::move(chain)), detail(std::move(det)) {}

    bool isError() const { return code != ErrorCode::SUCCESS; }
    
    std::string chainToString() const {
        if (dependencyChain.empty()) return "";
        std::string res;
        for (size_t i = 0; i < dependencyChain.size(); ++i) {
            res += dependencyChain[i];
            if (i + 1 < dependencyChain.size()) res += " -> ";
        }
        return res;
    }
};

// ============================================================================
// Generic Result<T> Monadic Container
// ============================================================================
template <typename T>
class Result {
private:
    std::optional<T> value_;
    Diagnostic diagnostic_;

public:
    static Result<T> ok(T val) {
        Result<T> r;
        r.value_ = std::move(val);
        return r;
    }

    static Result<T> err(Diagnostic diag) {
        Result<T> r;
        r.diagnostic_ = std::move(diag);
        return r;
    }

    static Result<T> err(ErrorCode code, std::string msg, std::string pkg = "") {
        return err(Diagnostic(code, std::move(pkg), std::move(msg)));
    }

    bool isOk() const { return value_.has_value(); }
    bool isErr() const { return !value_.has_value(); }

    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }

    const Diagnostic& error() const { return diagnostic_; }
};

// Void specialization for operations with no return value
template <>
class Result<void> {
private:
    Diagnostic diagnostic_;

public:
    static Result<void> ok() {
        return Result<void>();
    }

    static Result<void> err(Diagnostic diag) {
        Result<void> r;
        r.diagnostic_ = std::move(diag);
        return r;
    }

    static Result<void> err(ErrorCode code, std::string msg, std::string pkg = "") {
        return err(Diagnostic(code, std::move(pkg), std::move(msg)));
    }

    bool isOk() const { return !diagnostic_.isError(); }
    bool isErr() const { return diagnostic_.isError(); }

    const Diagnostic& error() const { return diagnostic_; }
};

// ============================================================================
// Configurable System Limits
// ============================================================================
struct Limits {
    static constexpr size_t MAX_PACKAGE_NAME_LENGTH = 64;
    static constexpr size_t MIN_PACKAGE_NAME_LENGTH = 1;
    static constexpr size_t MAX_VERSION_STRING_LENGTH = 64;
    static constexpr size_t MAX_CONSTRAINT_STRING_LENGTH = 256;
    static constexpr size_t MAX_PACKAGES_IN_REGISTRY = 100000;
    static constexpr size_t MAX_VERSIONS_PER_PACKAGE = 1000;
    static constexpr size_t MAX_DEPENDENCIES_PER_VERSION = 1000;
    static constexpr size_t MAX_BACKTRACK_STEPS = 50000;
    static constexpr size_t MAX_MULTIPLE_ORDERS = 100;
};

// ============================================================================
// Input Validation Functions
// ============================================================================
class Validator {
public:
    // Package names: 1-64 chars, [a-z0-9_-], cannot start/end with dot/dash/slash
    static Result<std::string> validatePackageName(std::string_view name) {
        if (name.empty()) {
            return Result<std::string>::err(ErrorCode::INVALID_PACKAGE_NAME, "Package name cannot be empty");
        }
        if (name.length() > Limits::MAX_PACKAGE_NAME_LENGTH) {
            return Result<std::string>::err(ErrorCode::INVALID_PACKAGE_NAME, "Package name exceeds maximum length of 64 chars");
        }
        for (char c : name) {
            bool validChar = (c >= 'a' && c <= 'z') ||
                             (c >= 'A' && c <= 'Z') ||
                             (c >= '0' && c <= '9') ||
                             (c == '_') || (c == '-') || (c == '.');
            if (!validChar) {
                return Result<std::string>::err(
                    ErrorCode::INVALID_PACKAGE_NAME,
                    std::string("Package name contains illegal character: '") + c + "'"
                );
            }
            if (c == '/' || c == '\\') {
                return Result<std::string>::err(
                    ErrorCode::INVALID_PACKAGE_NAME,
                    "Package name cannot contain path separators"
                );
            }
        }
        if (name.front() == '.' || name.front() == '-' || name.front() == '_') {
            return Result<std::string>::err(
                ErrorCode::INVALID_PACKAGE_NAME,
                "Package name cannot begin with '.', '-', or '_'"
            );
        }

        // Return normalized (lowercase) name for consistent hashing & interning
        std::string normalized;
        normalized.reserve(name.size());
        for (char c : name) {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return Result<std::string>::ok(normalized);
    }

    // Sanitize log and output text to eliminate control characters (prevent terminal injection)
    static std::string sanitizeText(std::string_view input) {
        std::string clean;
        clean.reserve(input.size());
        for (char c : input) {
            if (static_cast<unsigned char>(c) >= 32 && c != 127) {
                clean.push_back(c);
            } else if (c == '\t' || c == '\n') {
                clean.push_back(c);
            }
        }
        return clean;
    }
};

} // namespace DepResolver
