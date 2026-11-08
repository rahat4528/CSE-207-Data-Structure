#ifndef TTKV_CRAWLER_COMMON_H
#define TTKV_CRAWLER_COMMON_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <chrono>
#include <iostream>
#include <sstream>
#include <memory>
#include <functional>
#include <utility>
#include <stdexcept>

namespace crawler {

enum class StatusCode {
    OK = 0,
    INVALID_URL,
    UNSUPPORTED_SCHEME,
    SSRF_BLOCKED,
    ROBOTS_DISALLOWED,
    CONNECT_TIMEOUT,
    READ_TIMEOUT,
    MAX_REDIRECTS_EXCEEDED,
    RESPONSE_TOO_LARGE,
    HTTP_CLIENT_ERROR,
    HTTP_SERVER_ERROR,
    HTTP_TOO_MANY_REQUESTS,
    CRAWL_TRAP_DETECTED,
    PARSE_ERROR,
    CHECKPOINT_CORRUPTED,
    IO_ERROR,
    INTERNAL_ERROR
};

inline std::string_view statusCodeToString(StatusCode code) {
    switch (code) {
        case StatusCode::OK: return "OK";
        case StatusCode::INVALID_URL: return "INVALID_URL";
        case StatusCode::UNSUPPORTED_SCHEME: return "UNSUPPORTED_SCHEME";
        case StatusCode::SSRF_BLOCKED: return "SSRF_BLOCKED";
        case StatusCode::ROBOTS_DISALLOWED: return "ROBOTS_DISALLOWED";
        case StatusCode::CONNECT_TIMEOUT: return "CONNECT_TIMEOUT";
        case StatusCode::READ_TIMEOUT: return "READ_TIMEOUT";
        case StatusCode::MAX_REDIRECTS_EXCEEDED: return "MAX_REDIRECTS_EXCEEDED";
        case StatusCode::RESPONSE_TOO_LARGE: return "RESPONSE_TOO_LARGE";
        case StatusCode::HTTP_CLIENT_ERROR: return "HTTP_CLIENT_ERROR";
        case StatusCode::HTTP_SERVER_ERROR: return "HTTP_SERVER_ERROR";
        case StatusCode::HTTP_TOO_MANY_REQUESTS: return "HTTP_TOO_MANY_REQUESTS";
        case StatusCode::CRAWL_TRAP_DETECTED: return "CRAWL_TRAP_DETECTED";
        case StatusCode::PARSE_ERROR: return "PARSE_ERROR";
        case StatusCode::CHECKPOINT_CORRUPTED: return "CHECKPOINT_CORRUPTED";
        case StatusCode::IO_ERROR: return "IO_ERROR";
        case StatusCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

inline std::ostream& operator<<(std::ostream& os, StatusCode code) {
    return os << statusCodeToString(code);
}


template <typename T>
class Result {
public:
    static Result<T> ok(T val) {
        return Result<T>(std::move(val));
    }

    static Result<T> error(StatusCode code, std::string message) {
        return Result<T>(code, std::move(message));
    }

    bool isOk() const noexcept { return m_code == StatusCode::OK; }
    bool isError() const noexcept { return m_code != StatusCode::OK; }

    StatusCode code() const noexcept { return m_code; }
    const std::string& errorMessage() const noexcept { return m_errorMsg; }

    T& value() & {
        if (isError()) {
            throw std::runtime_error("Attempted to access value on error Result: " + m_errorMsg);
        }
        return m_value;
    }

    const T& value() const & {
        if (isError()) {
            throw std::runtime_error("Attempted to access value on error Result: " + m_errorMsg);
        }
        return m_value;
    }

    T&& value() && {
        if (isError()) {
            throw std::runtime_error("Attempted to access value on error Result: " + m_errorMsg);
        }
        return std::move(m_value);
    }

private:
    StatusCode m_code{StatusCode::OK};
    std::string m_errorMsg{};
    T m_value{};

    explicit Result(T val) : m_code(StatusCode::OK), m_value(std::move(val)) {}
    Result(StatusCode code, std::string msg) : m_code(code), m_errorMsg(std::move(msg)) {}
};

// Specialization for void
template <>
class Result<void> {
public:
    static Result<void> ok() {
        return Result<void>(StatusCode::OK, "");
    }

    static Result<void> error(StatusCode code, std::string message) {
        return Result<void>(code, std::move(message));
    }

    bool isOk() const noexcept { return m_code == StatusCode::OK; }
    bool isError() const noexcept { return m_code != StatusCode::OK; }

    StatusCode code() const noexcept { return m_code; }
    const std::string& errorMessage() const noexcept { return m_errorMsg; }

private:
    StatusCode m_code{StatusCode::OK};
    std::string m_errorMsg{};

    Result(StatusCode code, std::string msg) : m_code(code), m_errorMsg(std::move(msg)) {}
};

} // namespace crawler

#endif // TTKV_CRAWLER_COMMON_H
