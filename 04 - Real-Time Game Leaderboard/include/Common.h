#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace LeaderboardEngine {

using score_t = int64_t;
using timestamp_t = int64_t; // milliseconds since epoch

enum class ErrorCode {
    SUCCESS = 0,
    PLAYER_NOT_FOUND,
    DUPLICATE_PLAYER_ID,
    DUPLICATE_USERNAME,
    INVALID_PLAYER_ID,
    INVALID_USERNAME,
    INVALID_SCORE,
    SCORE_OUT_OF_BOUNDS,
    SCORE_JUMP_TOO_LARGE,
    DUPLICATE_EVENT_ID,
    EVENT_PAYLOAD_CONFLICT,
    BOARD_NOT_FOUND,
    INVALID_CATEGORY,
    INVALID_TIME_WINDOW,
    RATE_LIMIT_EXCEEDED,
    UNAUTHORIZED,
    FORBIDDEN,
    STORAGE_ERROR,
    CORRUPTED_DATA,
    INVALID_PAGINATION_OFFSET,
    INVALID_CURSOR,
    ALREADY_FRIENDS,
    NOT_FRIENDS,
    CANNOT_FRIEND_SELF,
    INTERNAL_ERROR
};

inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::PLAYER_NOT_FOUND: return "PLAYER_NOT_FOUND";
        case ErrorCode::DUPLICATE_PLAYER_ID: return "DUPLICATE_PLAYER_ID";
        case ErrorCode::DUPLICATE_USERNAME: return "DUPLICATE_USERNAME";
        case ErrorCode::INVALID_PLAYER_ID: return "INVALID_PLAYER_ID";
        case ErrorCode::INVALID_USERNAME: return "INVALID_USERNAME";
        case ErrorCode::INVALID_SCORE: return "INVALID_SCORE";
        case ErrorCode::SCORE_OUT_OF_BOUNDS: return "SCORE_OUT_OF_BOUNDS";
        case ErrorCode::SCORE_JUMP_TOO_LARGE: return "SCORE_JUMP_TOO_LARGE";
        case ErrorCode::DUPLICATE_EVENT_ID: return "DUPLICATE_EVENT_ID";
        case ErrorCode::EVENT_PAYLOAD_CONFLICT: return "EVENT_PAYLOAD_CONFLICT";
        case ErrorCode::BOARD_NOT_FOUND: return "BOARD_NOT_FOUND";
        case ErrorCode::INVALID_CATEGORY: return "INVALID_CATEGORY";
        case ErrorCode::INVALID_TIME_WINDOW: return "INVALID_TIME_WINDOW";
        case ErrorCode::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case ErrorCode::UNAUTHORIZED: return "UNAUTHORIZED";
        case ErrorCode::FORBIDDEN: return "FORBIDDEN";
        case ErrorCode::STORAGE_ERROR: return "STORAGE_ERROR";
        case ErrorCode::CORRUPTED_DATA: return "CORRUPTED_DATA";
        case ErrorCode::INVALID_PAGINATION_OFFSET: return "INVALID_PAGINATION_OFFSET";
        case ErrorCode::INVALID_CURSOR: return "INVALID_CURSOR";
        case ErrorCode::ALREADY_FRIENDS: return "ALREADY_FRIENDS";
        case ErrorCode::NOT_FRIENDS: return "NOT_FRIENDS";
        case ErrorCode::CANNOT_FRIEND_SELF: return "CANNOT_FRIEND_SELF";
        case ErrorCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

inline std::ostream& operator<<(std::ostream& os, ErrorCode code) {
    os << errorCodeToString(code);
    return os;
}

enum class ScoreUpdateMode {
    SET = 0, // Direct absolute assignment
    ADD = 1, // Delta accumulation
    MAX = 2  // Monotonic high-score only
};

inline const char* scoreUpdateModeToString(ScoreUpdateMode mode) {
    switch (mode) {
        case ScoreUpdateMode::SET: return "SET";
        case ScoreUpdateMode::ADD: return "ADD";
        case ScoreUpdateMode::MAX: return "MAX";
        default: return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, ScoreUpdateMode mode) {
    os << scoreUpdateModeToString(mode);
    return os;
}

enum class TimeWindow {
    ALL_TIME = 0,
    DAILY = 1,
    WEEKLY = 2,
    MONTHLY = 3
};

inline const char* timeWindowToString(TimeWindow win) {
    switch (win) {
        case TimeWindow::ALL_TIME: return "ALL_TIME";
        case TimeWindow::DAILY: return "DAILY";
        case TimeWindow::WEEKLY: return "WEEKLY";
        case TimeWindow::MONTHLY: return "MONTHLY";
        default: return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, TimeWindow win) {
    os << timeWindowToString(win);
    return os;
}

// Result container pattern
template <typename T>
struct Result {
    bool ok;
    T value;
    ErrorCode error;
    std::string message;

    static Result<T> Success(T val) {
        return {true, std::move(val), ErrorCode::SUCCESS, ""};
    }

    static Result<T> Failure(ErrorCode err, std::string msg) {
        return {false, T{}, err, std::move(msg)};
    }
};

template <>
struct Result<void> {
    bool ok;
    ErrorCode error;
    std::string message;

    static Result<void> Success() {
        return {true, ErrorCode::SUCCESS, ""};
    }

    static Result<void> Failure(ErrorCode err, std::string msg) {
        return {false, err, std::move(msg)};
    }
};

// Clock Abstraction for Deterministic Rollover & Time-Window Tests
class IClock {
public:
    virtual ~IClock() = default;
    virtual timestamp_t nowMs() const = 0;
};

class SystemClock : public IClock {
public:
    timestamp_t nowMs() const override {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
};

class FakeClock : public IClock {
private:
    timestamp_t currentMs_;

public:
    explicit FakeClock(timestamp_t initialMs = 0) : currentMs_(initialMs) {}

    timestamp_t nowMs() const override {
        return currentMs_;
    }

    void setTimeMs(timestamp_t ms) {
        currentMs_ = ms;
    }

    void advanceMs(int64_t deltaMs) {
        currentMs_ += deltaMs;
    }
};

// Validation & String Utilities
inline std::string toLower(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

inline bool isValidPlayerId(const std::string& id) {
    if (id.empty() || id.size() > 64) return false;
    for (char c : id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != ':') {
            return false;
        }
    }
    return true;
}

inline bool isValidUsername(const std::string& username) {
    if (username.empty() || username.size() > 32) return false;
    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

inline std::string sanitizeText(const std::string& input) {
    std::string clean;
    clean.reserve(input.size());
    for (char c : input) {
        if (c >= 32 && c <= 126) {
            clean += c;
        }
    }
    return clean;
}

// Standard CRC32 table for WAL integrity and snapshot verification
inline uint32_t calculateCrc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        crc ^= byte;
        for (int j = 0; j < 8; ++j) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

} // namespace LeaderboardEngine

#endif // COMMON_H
