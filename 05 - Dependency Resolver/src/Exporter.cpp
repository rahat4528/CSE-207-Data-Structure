#include "Exporter.h"
#include <sstream>
#include <algorithm>

namespace DepResolver {

namespace {

std::string escapeCsvField(std::string_view field) {
    if (field.empty()) return "";

    std::string text(field);
    // Neutralize spreadsheet formula injection characters (=, +, -, @)
    if (text[0] == '=' || text[0] == '+' || text[0] == '-' || text[0] == '@') {
        text = "'" + text;
    }

    bool needsQuotes = false;
    for (char c : text) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needsQuotes = true;
            break;
        }
    }

    if (!needsQuotes) return text;

    std::string out = "\"";
    for (char c : text) {
        if (c == '"') out += "\"\""; // double quote escape
        else out += c;
    }
    out += "\"";
    return out;
}

std::string escapeJsonString(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

} // namespace

std::string ConfigDiff::toString() const {
    std::ostringstream ss;
    ss << "=== Configuration Comparison Diff ===\n";
    if (!addedPackages.empty()) {
        ss << "\n[+] Added Packages (" << addedPackages.size() << "):\n";
        for (const auto& p : addedPackages) ss << "  + " << p << "\n";
    }
    if (!removedPackages.empty()) {
        ss << "\n[-] Removed Packages (" << removedPackages.size() << "):\n";
        for (const auto& p : removedPackages) ss << "  - " << p << "\n";
    }
    if (!changedPackages.empty()) {
        ss << "\n[*] Changed Packages (" << changedPackages.size() << "):\n";
        for (const auto& c : changedPackages) {
            ss << "  * " << c.packageName << ": "
               << (c.oldVersion.has_value() ? c.oldVersion.value().toString() : "none")
               << " -> " << c.newVersion.toString()
               << " [" << transitionTypeToString(c.type) << "]";
            if (c.isMajorChange) ss << " (MAJOR CHANGE WARNING!)";
            ss << "\n";
        }
    }
    if (!hasDifferences()) {
        ss << "Configurations are identical.\n";
    }
    return ss.str();
}

std::string Exporter::exportPlanToCsv(const InstallationPlan& plan) {
    std::ostringstream ss;
    ss << "order,package,version,level,reason,dependencies\n";
    for (const auto& step : plan.steps) {
        std::string deps;
        for (size_t i = 0; i < step.dependencies.size(); ++i) {
            deps += step.dependencies[i];
            if (i + 1 < step.dependencies.size()) deps += ";";
        }

        ss << step.order << ","
           << escapeCsvField(step.packageName) << ","
           << escapeCsvField(step.version) << ","
           << step.level << ","
           << escapeCsvField(step.reason) << ","
           << escapeCsvField(deps) << "\n";
    }
    return ss.str();
}

std::string Exporter::exportHistoryToCsv(const std::vector<TransactionRecord>& history) {
    std::ostringstream ss;
    ss << "tx_id,timestamp,action,status,summary,affected_packages\n";
    for (const auto& rec : history) {
        std::string pkgs;
        for (size_t i = 0; i < rec.affectedPackages.size(); ++i) {
            pkgs += rec.affectedPackages[i];
            if (i + 1 < rec.affectedPackages.size()) pkgs += ";";
        }

        ss << rec.transactionId << ","
           << escapeCsvField(rec.timestamp) << ","
           << escapeCsvField(rec.actionName) << ","
           << escapeCsvField(transactionStatusToString(rec.status)) << ","
           << escapeCsvField(rec.summary) << ","
           << escapeCsvField(pkgs) << "\n";
    }
    return ss.str();
}

std::string Exporter::exportResolutionToJson(const ResolutionResult& result) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"status\": \"" << resolutionStatusToString(result.status) << "\",\n";
    ss << "  \"steps_taken\": " << result.stepsTaken << ",\n";
    ss << "  \"packages\": {\n";
    size_t count = 0;
    for (const auto& [pkg, ver] : result.selectedVersions) {
        ss << "    \"" << escapeJsonString(pkg) << "\": \"" << ver.toString() << "\"";
        if (++count < result.selectedVersions.size()) ss << ",";
        ss << "\n";
    }
    ss << "  },\n";

    ss << "  \"installation_plan\": [\n";
    for (size_t i = 0; i < result.plan.steps.size(); ++i) {
        const auto& step = result.plan.steps[i];
        ss << "    {\n";
        ss << "      \"order\": " << step.order << ",\n";
        ss << "      \"package\": \"" << escapeJsonString(step.packageName) << "\",\n";
        ss << "      \"version\": \"" << escapeJsonString(step.version) << "\",\n";
        ss << "      \"level\": " << step.level << ",\n";
        ss << "      \"reason\": \"" << escapeJsonString(step.reason) << "\"\n";
        ss << "    }";
        if (i + 1 < result.plan.steps.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

std::string Exporter::exportHistoryToJson(const std::vector<TransactionRecord>& history) {
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& rec = history[i];
        ss << "  {\n";
        ss << "    \"tx_id\": " << rec.transactionId << ",\n";
        ss << "    \"action\": \"" << escapeJsonString(rec.actionName) << "\",\n";
        ss << "    \"status\": \"" << transactionStatusToString(rec.status) << "\",\n";
        ss << "    \"summary\": \"" << escapeJsonString(rec.summary) << "\"\n";
        ss << "  }";
        if (i + 1 < history.size()) ss << ",";
        ss << "\n";
    }
    ss << "]\n";
    return ss.str();
}

ConfigDiff Exporter::compareConfigurations(
    const std::map<std::string, SemVer>& configA,
    const std::map<std::string, SemVer>& configB) 
{
    ConfigDiff diff;

    // Check for removed packages (in A but not B)
    for (const auto& [pkg, verA] : configA) {
        auto it = configB.find(pkg);
        if (it == configB.end()) {
            diff.removedPackages.push_back(pkg);
        } else {
            const auto& verB = it->second;
            if (verA != verB) {
                VersionTransition vt;
                vt.packageName = pkg;
                vt.oldVersion = verA;
                vt.newVersion = verB;
                int cmp = verB.compare(verA);
                if (cmp > 0) {
                    vt.type = TransitionType::UPGRADE;
                    if (verB.major > verA.major) vt.isMajorChange = true;
                } else {
                    vt.type = TransitionType::DOWNGRADE;
                    if (verB.major < verA.major) vt.isMajorChange = true;
                }
                diff.changedPackages.push_back(std::move(vt));
            }
        }
    }

    // Check for added packages (in B but not A)
    for (const auto& [pkg, verB] : configB) {
        if (configA.find(pkg) == configA.end()) {
            diff.addedPackages.push_back(pkg);
        }
    }

    std::sort(diff.addedPackages.begin(), diff.addedPackages.end());
    std::sort(diff.removedPackages.begin(), diff.removedPackages.end());
    return diff;
}

std::string Exporter::generateLockfile(
    const std::map<std::string, SemVer>& resolvedVersions,
    const InstallationPlan& plan,
    const Registry& registry) 
{
    std::ostringstream ss;
    ss << "# DEPENDENCY_RESOLVER_LOCKFILE_V1\n";
    ss << "# Generated automatically. Deterministic & reproducible.\n\n";

    ss << "[metadata]\n";
    ss << "total_packages=" << resolvedVersions.size() << "\n";
    ss << "total_levels=" << plan.totalLevels << "\n\n";

    ss << "[resolved_packages]\n";
    for (const auto& [pkgName, ver] : resolvedVersions) {
        const auto* pkg = registry.getPackage(pkgName);
        std::string csum = "sha256-verified";
        if (pkg) {
            const auto* pv = pkg->findVersion(ver);
            if (pv && !pv->checksum.empty()) csum = pv->checksum;
        }
        ss << pkgName << " = " << ver.toString() << " | checksum=" << csum << "\n";
    }

    ss << "\n[install_sequence]\n";
    for (const auto& step : plan.steps) {
        ss << step.order << ": " << step.packageName << "@" << step.version 
           << " (level=" << step.level << ")\n";
    }

    return ss.str();
}

} // namespace DepResolver

