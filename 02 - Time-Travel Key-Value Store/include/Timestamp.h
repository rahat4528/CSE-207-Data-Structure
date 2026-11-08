#pragma once

#include "Common.h"
#include <string>
#include <cstdint>

namespace ttkv {

class Timestamp {
public:
    // Parses an ISO 8601 string into UTC microseconds since Unix Epoch.
    // Handles 'Z', numeric offsets (+HH:MM / -HH:MM), fractional seconds, and space/T separators.
    // Rejects impossible calendar dates (e.g. Feb 30, month 13, hour 25).
    static Result<uint64_t> parseIso8601(const std::string& str);

    // Formats UTC microseconds into ISO 8601 string: YYYY-MM-DDTHH:MM:SSZ (or with .ffffffZ if requested)
    static std::string formatIso8601(uint64_t micros, bool includeFractional = false);

    // Checks whether a year is a Gregorian leap year
    static bool isLeapYear(int64_t year);

    // Returns the number of days in a given year and month (1-12)
    static int daysInMonth(int64_t year, int month);
};

} // namespace ttkv
