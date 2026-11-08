#include "PathSanitizer.h"
#include <algorithm>
#include <cctype>

namespace HuffmanEngine {

std::string PathSanitizer::sanitizeFilename(const std::string& rawName) {
    if (rawName.empty()) return "restored_file.bin";

    std::filesystem::path p(rawName);
    std::string base = p.filename().string();

    // Remove any remaining path separators or traversal markers
    std::string clean;
    clean.reserve(base.size());
    for (char c : base) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            clean.push_back('_');
        } else if (static_cast<unsigned char>(c) < 32) {
            clean.push_back('_'); // Strip control characters
        } else {
            clean.push_back(c);
        }
    }

    if (clean.empty() || clean == "." || clean == "..") {
        return "restored_file.bin";
    }

    // Windows reserved device names check: CON, PRN, AUX, NUL, COM1-9, LPT1-9
    std::string upper = clean;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    size_t dotPos = upper.find('.');
    std::string stem = (dotPos == std::string::npos) ? upper : upper.substr(0, dotPos);

    static const char* reserved[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    for (const char* res : reserved) {
        if (stem == res) {
            clean = "safe_" + clean;
            break;
        }
    }

    return clean;
}

Result<std::filesystem::path> PathSanitizer::resolveSafeOutputPath(
    const std::filesystem::path& baseDir,
    const std::string& declaredFilename) {

    std::string safeName = sanitizeFilename(declaredFilename);
    std::filesystem::path target = baseDir / safeName;

    // Verify it stays strictly within baseDir
    std::error_code ec;
    auto canonicalBase = std::filesystem::weakly_canonical(baseDir, ec);
    auto canonicalTarget = std::filesystem::weakly_canonical(target, ec);

    std::string baseStr = canonicalBase.string();
    std::string targetStr = canonicalTarget.string();

    if (targetStr.rfind(baseStr, 0) != 0) {
        return Result<std::filesystem::path>::fail(ErrorCode::ERR_PATH_TRAVERSAL, "Security error: target path escapes output directory!");
    }

    return Result<std::filesystem::path>::ok(target);
}

Result<void> PathSanitizer::validateInputFile(const std::filesystem::path& inputPath) {
    if (!std::filesystem::exists(inputPath)) {
        return Result<void>::fail(ErrorCode::ERR_FILE_NOT_FOUND, "Input file does not exist: " + inputPath.string());
    }
    if (std::filesystem::is_directory(inputPath)) {
        return Result<void>::fail(ErrorCode::ERR_INVALID_PARAMETER, "Input path is a directory, not a regular file: " + inputPath.string());
    }
    if (!std::filesystem::is_regular_file(inputPath)) {
        return Result<void>::fail(ErrorCode::ERR_INVALID_PARAMETER, "Input path is not a regular file: " + inputPath.string());
    }
    return Result<void>::ok();
}

Result<void> PathSanitizer::validateOutputFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    bool forceOverwrite) {

    std::error_code ec;
    auto canonIn = std::filesystem::weakly_canonical(inputPath, ec);
    auto canonOut = std::filesystem::weakly_canonical(outputPath, ec);

    // Rule: Output path must never equal input path
    if (canonIn == canonOut) {
        return Result<void>::fail(ErrorCode::ERR_INVALID_PARAMETER, "Output path cannot be identical to input path (would overwrite source)!");
    }

    // Rule: Refuse to overwrite existing files without explicit force flag
    if (std::filesystem::exists(outputPath) && !forceOverwrite) {
        return Result<void>::fail(ErrorCode::ERR_FILE_EXISTS, "Output file already exists: " + outputPath.string() + " (use --force / -f to overwrite)");
    }

    return Result<void>::ok();
}

} // namespace HuffmanEngine
