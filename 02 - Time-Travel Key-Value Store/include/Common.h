#pragma once

#include <string>
#include <cstdint>
#include <iostream>
#include <utility>
#include <optional>

namespace ttkv {

// System-wide error codes
enum class ErrorCode : uint32_t {
    OK = 0,
    ERR_KEY_UNKNOWN = 1,
    ERR_KEY_DELETED = 2,
    ERR_NOT_YET_EXISTS = 3,
    ERR_EXPIRED = 4,
    ERR_PRUNED = 5,
    ERR_INVALID_VERSION = 6,
    ERR_INVALID_TIMESTAMP = 7,
    ERR_KEY_INVALID = 8,
    ERR_VALUE_INVALID = 9,
    ERR_TRANSACTION_ACTIVE = 10,
    ERR_NO_ACTIVE_TRANSACTION = 11,
    ERR_CORRUPTED_DATA = 12,
    ERR_IO_FAILURE = 13,
    ERR_DUPLICATE_VERSION_ID = 14,
    ERR_SNAPSHOT_NOT_FOUND = 15,
    ERR_SNAPSHOT_ALREADY_EXISTS = 16,
    ERR_CHECKSUM_MISMATCH = 17,
    ERR_UNSUPPORTED_FORMAT = 18,
    ERR_SYNTAX_ERROR = 19
};

// Precise Query Result Semantics (Section 6.3)
enum class QueryStatus : uint32_t {
    FOUND = 0,             // Key exists with a non-deleted value at requested time/version
    DELETED = 1,           // The effective version is a tombstone
    NOT_YET_EXISTS = 2,    // The key exists, but the query time is before its initial creation
    KEY_UNKNOWN = 3,       // The key never existed in the store
    EXPIRED = 4,           // TTL expired as of the query time
    PRUNED = 5,            // The requested historical version was removed by retention policy
    INVALID_VERSION = 6,   // Version number is invalid (<=0, out of range)
    INVALID_TIME = 7       // Malformed or out-of-range timestamp
};

// Write operation types recorded in immutable version records and WAL
enum class OpType : uint8_t {
    OP_SET = 1,            // Initial creation of a key
    OP_UPDATE = 2,         // Update to an existing key
    OP_DELETE = 3,         // Deletion creating a tombstone version
    OP_ROLLBACK = 4,       // Rollback creating a new version restoring prior state
    OP_EXPIRE = 5,         // TTL expiration recorded as an event
    OP_IMPORT = 6          // Bulk import of historical record
};

inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK: return "OK";
        case ErrorCode::ERR_KEY_UNKNOWN: return "Key unknown";
        case ErrorCode::ERR_KEY_DELETED: return "Key deleted";
        case ErrorCode::ERR_NOT_YET_EXISTS: return "Key did not exist yet at specified time";
        case ErrorCode::ERR_EXPIRED: return "Key expired";
        case ErrorCode::ERR_PRUNED: return "Version pruned by retention policy";
        case ErrorCode::ERR_INVALID_VERSION: return "Invalid version number";
        case ErrorCode::ERR_INVALID_TIMESTAMP: return "Invalid or unparseable timestamp";
        case ErrorCode::ERR_KEY_INVALID: return "Invalid key format or length";
        case ErrorCode::ERR_VALUE_INVALID: return "Invalid value format or size exceeds limit";
        case ErrorCode::ERR_TRANSACTION_ACTIVE: return "A transaction is already active";
        case ErrorCode::ERR_NO_ACTIVE_TRANSACTION: return "No transaction active";
        case ErrorCode::ERR_CORRUPTED_DATA: return "Corrupted data or file truncation detected";
        case ErrorCode::ERR_IO_FAILURE: return "I/O failure during disk operation";
        case ErrorCode::ERR_DUPLICATE_VERSION_ID: return "Duplicate version ID rejected";
        case ErrorCode::ERR_SNAPSHOT_NOT_FOUND: return "Snapshot not found";
        case ErrorCode::ERR_SNAPSHOT_ALREADY_EXISTS: return "Snapshot with this name already exists";
        case ErrorCode::ERR_CHECKSUM_MISMATCH: return "CRC32 checksum mismatch";
        case ErrorCode::ERR_UNSUPPORTED_FORMAT: return "Unsupported file or schema format";
        case ErrorCode::ERR_SYNTAX_ERROR: return "Command syntax error";
        default: return "Unknown error";
    }
}

inline const char* queryStatusToString(QueryStatus status) {
    switch (status) {
        case QueryStatus::FOUND: return "FOUND";
        case QueryStatus::DELETED: return "DELETED";
        case QueryStatus::NOT_YET_EXISTS: return "NOT_YET_EXISTS";
        case QueryStatus::KEY_UNKNOWN: return "KEY_UNKNOWN";
        case QueryStatus::EXPIRED: return "EXPIRED";
        case QueryStatus::PRUNED: return "PRUNED";
        case QueryStatus::INVALID_VERSION: return "INVALID_VERSION";
        case QueryStatus::INVALID_TIME: return "INVALID_TIME";
        default: return "UNKNOWN";
    }
}

inline const char* opTypeToString(OpType op) {
    switch (op) {
        case OpType::OP_SET: return "SET";
        case OpType::OP_UPDATE: return "UPDATE";
        case OpType::OP_DELETE: return "DELETE";
        case OpType::OP_ROLLBACK: return "ROLLBACK";
        case OpType::OP_EXPIRE: return "EXPIRE";
        case OpType::OP_IMPORT: return "IMPORT";
        default: return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, ErrorCode code) {
    os << errorCodeToString(code);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, QueryStatus status) {
    os << queryStatusToString(status);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, OpType op) {
    os << opTypeToString(op);
    return os;
}

// Result<T> Monad for explicit error propagation without exceptions
template <typename T>
class Result {
public:
    Result(T value) : m_value(std::move(value)), m_error(ErrorCode::OK), m_message("") {}
    Result(ErrorCode err, std::string msg = "") : m_value(std::nullopt), m_error(err), m_message(std::move(msg)) {}

    static Result<T> ok(T value) {
        return Result<T>(std::move(value));
    }

    static Result<T> error(ErrorCode err, std::string msg = "") {
        return Result<T>(err, std::move(msg));
    }

    bool isOk() const { return m_error == ErrorCode::OK; }
    bool isError() const { return m_error != ErrorCode::OK; }

    const T& value() const { return m_value.value(); }
    T& value() { return m_value.value(); }

    ErrorCode error() const { return m_error; }
    const std::string& message() const { return m_message; }

private:
    std::optional<T> m_value;
    ErrorCode m_error;
    std::string m_message;
};

// Specialization for void results
template <>
class Result<void> {
public:
    Result() : m_error(ErrorCode::OK), m_message("") {}
    Result(ErrorCode err, std::string msg = "") : m_error(err), m_message(std::move(msg)) {}

    static Result<void> ok() { return Result<void>(); }
    static Result<void> error(ErrorCode err, std::string msg = "") {
        return Result<void>(err, std::move(msg));
    }

    bool isOk() const { return m_error == ErrorCode::OK; }
    bool isError() const { return m_error != ErrorCode::OK; }

    ErrorCode error() const { return m_error; }
    const std::string& message() const { return m_message; }

private:
    ErrorCode m_error;
    std::string m_message;
};

} // namespace ttkv
