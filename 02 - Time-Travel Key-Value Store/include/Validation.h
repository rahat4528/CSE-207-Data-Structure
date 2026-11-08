#pragma once

#include "Common.h"
#include <string>
#include <string_view>
#include <cstdint>

namespace ttkv {

class Validation {
public:
    static constexpr size_t kMaxKeyLength = 256;
    static constexpr size_t kMaxValueLength = 10 * 1024 * 1024; // 10 MB limit

    // Validates a key according to system constraints
    static Result<void> validateKey(std::string_view key);

    // Validates a value size limit
    static Result<void> validateValue(std::string_view value);

    // Validates a version number (> 0)
    static Result<void> validateVersionNumber(uint32_t version);

    // Validates a snapshot identifier
    static Result<void> validateSnapshotName(std::string_view name);
};

} // namespace ttkv
