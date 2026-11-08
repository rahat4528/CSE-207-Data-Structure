#include "Config.h"
#include <algorithm>

namespace crawler {

Result<void> Config::validate() const {
    if (seedUrls.empty()) {
        return Result<void>::error(StatusCode::INVALID_URL, "At least one seed URL must be specified");
    }
    if (concurrency == 0) {
        return Result<void>::error(StatusCode::INTERNAL_ERROR, "Concurrency must be >= 1");
    }
    if (bloomCapacity == 0) {
        return Result<void>::error(StatusCode::INTERNAL_ERROR, "Bloom filter capacity must be > 0");
    }
    if (bloomTargetFpr <= 0.0 || bloomTargetFpr >= 1.0) {
        return Result<void>::error(StatusCode::INTERNAL_ERROR, "Bloom filter target FPR must be strictly between 0 and 1");
    }
    if (maxResponseSizeBytes == 0) {
        return Result<void>::error(StatusCode::INTERNAL_ERROR, "Max response size must be > 0");
    }
    return Result<void>::ok();
}

Result<Config> Config::parseCommandLine(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) {
            cfg.seedUrls.push_back(argv[++i]);
        } else if (arg == "--depth" && i + 1 < argc) {
            cfg.maxDepth = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--pages" && i + 1 < argc) {
            cfg.maxPages = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--concurrency" && i + 1 < argc) {
            cfg.concurrency = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--delay" && i + 1 < argc) {
            cfg.perHostDelayMs = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--domain-mode" && i + 1 < argc) {
            auto rMode = domainModeFromString(argv[++i]);
            if (rMode.isError()) return Result<Config>::error(rMode.code(), rMode.errorMessage());
            cfg.domainMode = rMode.value();
        } else if (arg == "--checkpoint" && i + 1 < argc) {
            cfg.checkpointPath = argv[++i];
        } else if (arg == "--no-robots") {
            cfg.respectRobots = false;
        } else if (arg == "--no-simhash") {
            cfg.enableSimHash = false;
        } else if (arg == "--allow-loopback") {
            cfg.allowTestLoopback = true;
        } else if (arg == "--user-agent" && i + 1 < argc) {
            cfg.userAgent = argv[++i];
        } else if (arg == "--export-json" && i + 1 < argc) {
            cfg.exportJsonPath = argv[++i];
        } else if (arg == "--export-csv" && i + 1 < argc) {
            cfg.exportCsvPath = argv[++i];
        } else if (arg == "--export-dot" && i + 1 < argc) {
            cfg.exportDotPath = argv[++i];
        } else if (arg == "--export-html" && i + 1 < argc) {
            cfg.exportHtmlPath = argv[++i];
        } else if (arg == "--export-report" && i + 1 < argc) {
            cfg.exportReportPath = argv[++i];
        } else if (arg == "--bloom-capacity" && i + 1 < argc) {
            cfg.bloomCapacity = std::stoull(argv[++i]);
        } else if (arg == "--bloom-fpr" && i + 1 < argc) {
            cfg.bloomTargetFpr = std::stod(argv[++i]);
        } else if (arg == "--demo") {
            cfg.runDemo = true;
        }
    }
    return Result<Config>::ok(cfg);
}

} // namespace crawler

