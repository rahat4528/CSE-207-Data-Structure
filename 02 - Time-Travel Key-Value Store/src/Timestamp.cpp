#include "Timestamp.h"
#include <sstream>
#include <iomanip>
#include <cctype>

namespace ttkv {

namespace {

// Howard Hinnant's algorithm for converting (year, month, day) to days since 1970-01-01
int64_t daysFromCivil(int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

// Howard Hinnant's algorithm for converting days since 1970-01-01 to (year, month, day)
void civilFromDays(int64_t z, int64_t& y, unsigned& m, unsigned& d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int64_t>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += (m <= 2);
}

} // namespace

bool Timestamp::isLeapYear(int64_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int Timestamp::daysInMonth(int64_t year, int month) {
    if (month < 1 || month > 12) return 0;
    static const int kDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return kDays[month - 1];
}

Result<uint64_t> Timestamp::parseIso8601(const std::string& input) {
    if (input.empty()) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Timestamp string is empty");
    }

    // Expected format: YYYY-MM-DDTHH:MM:SS[.ffffff][Z|(+|-)HH:MM]
    // Minimum length: 19 characters (e.g. 2026-11-01T10:30:00)
    if (input.size() < 19) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Timestamp string too short");
    }

    size_t idx = 0;
    auto readDigits = [&](size_t count, int& out) -> bool {
        if (idx + count > input.size()) return false;
        out = 0;
        for (size_t i = 0; i < count; ++i) {
            char c = input[idx + i];
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            out = out * 10 + (c - '0');
        }
        idx += count;
        return true;
    };

    auto expectChar = [&](char expected) -> bool {
        if (idx < input.size() && input[idx] == expected) {
            idx++;
            return true;
        }
        return false;
    };

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (!readDigits(4, year)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid year");
    if (!expectChar('-')) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Expected '-' after year");
    if (!readDigits(2, month)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid month");
    if (!expectChar('-')) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Expected '-' after month");
    if (!readDigits(2, day)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid day");

    // Separator between date and time: 'T', 't', or ' '
    if (idx >= input.size() || (input[idx] != 'T' && input[idx] != 't' && input[idx] != ' ')) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Expected 'T' or space separator");
    }
    idx++;

    if (!readDigits(2, hour)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid hour");
    if (!expectChar(':')) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Expected ':' after hour");
    if (!readDigits(2, minute)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid minute");
    if (!expectChar(':')) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Expected ':' after minute");
    if (!readDigits(2, second)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid second");

    // Calendar validations
    if (month < 1 || month > 12) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Month out of range (1-12)");
    }
    int maxDay = daysInMonth(year, month);
    if (day < 1 || day > maxDay) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Day out of range for month");
    }
    if (hour < 0 || hour > 23) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Hour out of range (0-23)");
    }
    if (minute < 0 || minute > 59) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Minute out of range (0-59)");
    }
    if (second < 0 || second > 59) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Second out of range (0-59)");
    }

    // Optional fractional seconds (.123456)
    uint64_t fractionalMicros = 0;
    if (idx < input.size() && input[idx] == '.') {
        idx++;
        uint64_t mult = 100000;
        int digitsRead = 0;
        while (idx < input.size() && std::isdigit(static_cast<unsigned char>(input[idx]))) {
            if (digitsRead < 6) {
                fractionalMicros += static_cast<uint64_t>(input[idx] - '0') * mult;
                mult /= 10;
            }
            digitsRead++;
            idx++;
        }
        if (digitsRead == 0) {
            return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Expected digits after decimal point");
        }
    }

    // Timezone parsing: 'Z', 'z', or (+|-)HH:MM or end of string (assumes UTC)
    int64_t offsetSeconds = 0;
    if (idx < input.size()) {
        char tz = input[idx];
        if (tz == 'Z' || tz == 'z') {
            idx++;
        } else if (tz == '+' || tz == '-') {
            idx++;
            int tzHour = 0, tzMin = 0;
            if (!readDigits(2, tzHour)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid timezone hour");
            if (expectChar(':')) {
                if (!readDigits(2, tzMin)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid timezone minute");
            } else if (idx < input.size() && std::isdigit(static_cast<unsigned char>(input[idx]))) {
                if (!readDigits(2, tzMin)) return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Invalid timezone minute");
            }
            if (tzHour < 0 || tzHour > 23 || tzMin < 0 || tzMin > 59) {
                return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Timezone offset out of range");
            }
            int64_t totalOffsetSec = tzHour * 3600 + tzMin * 60;
            offsetSeconds = (tz == '+') ? totalOffsetSec : -totalOffsetSec;
        } else {
            return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Unexpected character in timezone");
        }
    }

    if (idx != input.size()) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Extraneous trailing characters in timestamp");
    }

    int64_t days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    if (days < 0) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Dates prior to 1970-01-01 are out of range");
    }

    int64_t totalSeconds = days * 86400LL + hour * 3600LL + minute * 60LL + second;
    // Apply timezone offset to convert local time to UTC: UTC = local - offset
    totalSeconds -= offsetSeconds;

    if (totalSeconds < 0) {
        return Result<uint64_t>::error(ErrorCode::ERR_INVALID_TIMESTAMP, "Computed UTC timestamp is prior to 1970-01-01");
    }

    uint64_t totalMicros = static_cast<uint64_t>(totalSeconds) * 1000000ULL + fractionalMicros;
    return Result<uint64_t>::ok(totalMicros);
}

std::string Timestamp::formatIso8601(uint64_t micros, bool includeFractional) {
    int64_t totalSeconds = static_cast<int64_t>(micros / 1000000ULL);
    uint32_t remainderMicros = static_cast<uint32_t>(micros % 1000000ULL);

    int64_t days = totalSeconds / 86400LL;
    int secondsOfDay = static_cast<int>(totalSeconds % 86400LL);

    int hour = secondsOfDay / 3600;
    int minute = (secondsOfDay % 3600) / 60;
    int second = secondsOfDay % 60;

    int64_t year = 0;
    unsigned month = 0, day = 0;
    civilFromDays(days, year, month, day);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << year << "-"
        << std::setw(2) << month << "-"
        << std::setw(2) << day << "T"
        << std::setw(2) << hour << ":"
        << std::setw(2) << minute << ":"
        << std::setw(2) << second;

    if (includeFractional && remainderMicros > 0) {
        oss << "." << std::setw(6) << remainderMicros;
    }
    oss << "Z";

    return oss.str();
}

} // namespace ttkv
