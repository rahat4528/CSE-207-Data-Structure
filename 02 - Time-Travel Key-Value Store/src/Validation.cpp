#include "Validation.h"
#include <cctype>

namespace ttkv {

Result<void> Validation::validateKey(std::string_view key) {
    if (key.empty()) {
        return Result<void>::error(ErrorCode::ERR_KEY_INVALID, "Key cannot be empty");
    }
    if (key.size() > kMaxKeyLength) {
        return Result<void>::error(ErrorCode::ERR_KEY_INVALID, "Key length exceeds maximum of 256 characters");
    }
    for (char c : key) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 32 || uc == 127) { // Disallow control characters
            return Result<void>::error(ErrorCode::ERR_KEY_INVALID, "Key contains invalid control characters");
        }
    }
    return Result<void>::ok();
}

Result<void> Validation::validateValue(std::string_view value) {
    if (value.size() > kMaxValueLength) {
        return Result<void>::error(ErrorCode::ERR_VALUE_INVALID, "Value size exceeds maximum of 10 MB");
    }
    return Result<void>::ok();
}

Result<void> Validation::validateVersionNumber(uint32_t version) {
    if (version == 0) {
        return Result<void>::error(ErrorCode::ERR_INVALID_VERSION, "Version number must be greater than zero (1-based)");
    }
    return Result<void>::ok();
}

Result<void> Validation::validateSnapshotName(std::string_view name) {
    if (name.empty()) {
        return Result<void>::error(ErrorCode::ERR_SYNTAX_ERROR, "Snapshot name cannot be empty");
    }
    if (name.size() > 64) {
        return Result<void>::error(ErrorCode::ERR_SYNTAX_ERROR, "Snapshot name exceeds maximum of 64 characters");
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != '.') {
            return Result<void>::error(ErrorCode::ERR_SYNTAX_ERROR,
                                       "Snapshot name may only contain alphanumeric characters, underscores, hyphens, and dots");
        }
    }
    return Result<void>::ok();
}

} // namespace ttkv
