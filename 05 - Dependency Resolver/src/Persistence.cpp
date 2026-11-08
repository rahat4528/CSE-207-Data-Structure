#include "Persistence.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace DepResolver {

namespace {

std::string trimStr(std::string_view sv) {
    size_t first = 0;
    while (first < sv.size() && std::isspace(static_cast<unsigned char>(sv[first]))) first++;
    if (first == sv.size()) return "";
    size_t last = sv.size() - 1;
    while (last > first && std::isspace(static_cast<unsigned char>(sv[last]))) last--;
    return std::string(sv.substr(first, last - first + 1));
}

} // namespace

uint64_t Persistence::computeChecksum(std::string_view text) {
    // 64-bit FNV-1a hash
    uint64_t hash = 14695981039346656037ULL;
    for (char c : text) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

Result<void> Persistence::saveRegistry(const Registry& registry, const std::string& filePath) {
    std::string tmpPath = filePath + ".tmp";
    std::ofstream out(tmpPath, std::ios::trunc);
    if (!out.is_open()) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Cannot open file for writing: " + tmpPath);
    }

    out << "# REGISTRY_MANIFEST_V1\n";
    for (const auto* pkg : registry.getAllPackagesSorted()) {
        out << "PACKAGE " << pkg->name() << " | " << pkg->description() << "\n";
        for (const auto& pv : pkg->versions()) {
            out << "VERSION " << pv.version.toString() << "\n";
            for (const auto& dep : pv.dependencies) {
                out << "DEP " << dep.packageName << " | " << dep.constraint.toString() 
                    << " | " << dependencyKindToString(dep.kind) << "\n";
            }
        }
    }

    out.flush();
    out.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Atomic rename failed: " + ec.message());
    }

    return Result<void>::ok();
}

Result<Registry> Persistence::loadRegistry(const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        return Result<Registry>::err(ErrorCode::IO_ERROR, "Cannot open file for reading: " + filePath);
    }

    Registry registry;
    std::string line;
    std::string currentPkg;
    std::optional<PackageVersion> currentVer;

    size_t lineNum = 0;
    while (std::getline(in, line)) {
        lineNum++;
        line = trimStr(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "PACKAGE") {
            if (currentVer.has_value() && !currentPkg.empty()) {
                registry.registerVersion(currentPkg, currentVer.value());
                currentVer = std::nullopt;
            }
            std::string name;
            ss >> name;
            std::string remaining;
            std::getline(ss, remaining);
            std::string desc;
            size_t bar = remaining.find('|');
            if (bar != std::string::npos) desc = trimStr(remaining.substr(bar + 1));

            currentPkg = name;
            registry.registerPackage(name, desc);
        } else if (tag == "VERSION") {
            if (currentVer.has_value() && !currentPkg.empty()) {
                registry.registerVersion(currentPkg, currentVer.value());
                currentVer = std::nullopt;
            }
            std::string verStr;
            ss >> verStr;
            auto vRes = SemVer::parse(verStr);
            if (vRes.isErr()) {
                return Result<Registry>::err(ErrorCode::PARSE_ERROR, "Invalid version on line " + std::to_string(lineNum) + ": " + verStr);
            }
            currentVer = PackageVersion(vRes.value());
        } else if (tag == "DEP") {
            if (!currentVer.has_value()) {
                return Result<Registry>::err(ErrorCode::PARSE_ERROR, "DEP tag without VERSION on line " + std::to_string(lineNum));
            }
            std::string depName;
            ss >> depName;
            std::string remaining;
            std::getline(ss, remaining);
            std::string constrStr = "*";
            DependencyKind kind = DependencyKind::RUNTIME_DEP;

            // Split by '|'
            size_t bar1 = remaining.find('|');
            if (bar1 != std::string::npos) {
                size_t bar2 = remaining.find('|', bar1 + 1);
                if (bar2 != std::string::npos) {
                    constrStr = trimStr(remaining.substr(bar1 + 1, bar2 - bar1 - 1));
                    std::string kindStr = trimStr(remaining.substr(bar2 + 1));
                    if (kindStr == "dev") kind = DependencyKind::DEVELOPMENT_DEP;
                    else if (kindStr == "optional") kind = DependencyKind::OPTIONAL_DEP;
                    else if (kindStr == "conflict") kind = DependencyKind::CONFLICT_DEP;
                } else {
                    constrStr = trimStr(remaining.substr(bar1 + 1));
                }
            }

            auto cRes = Constraint::parse(constrStr);
            if (cRes.isErr()) {
                return Result<Registry>::err(ErrorCode::PARSE_ERROR, "Invalid constraint on line " + std::to_string(lineNum) + ": " + constrStr);
            }
            currentVer->dependencies.emplace_back(depName, cRes.value(), kind);
        }
    }

    if (currentVer.has_value() && !currentPkg.empty()) {
        registry.registerVersion(currentPkg, currentVer.value());
    }

    return Result<Registry>::ok(registry);
}

Result<void> Persistence::saveInstalledState(const InstalledState& state, const std::string& filePath) {
    std::string tmpPath = filePath + ".tmp";
    std::ofstream out(tmpPath, std::ios::trunc);
    if (!out.is_open()) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Cannot open file for writing: " + tmpPath);
    }

    out << "# INSTALLED_STATE_V1\n";
    for (const auto& [name, pkg] : state.getAll()) {
        out << "INSTALLED " << pkg.name << " " << pkg.version.toString() << " " 
            << installReasonToString(pkg.reason) << " " << pkg.transactionId << "\n";
    }

    out.flush();
    out.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Atomic rename failed: " + ec.message());
    }

    return Result<void>::ok();
}

Result<InstalledState> Persistence::loadInstalledState(const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        return Result<InstalledState>::err(ErrorCode::IO_ERROR, "Cannot open file: " + filePath);
    }

    InstalledState state;
    std::string line;
    while (std::getline(in, line)) {
        line = trimStr(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "INSTALLED") {
            std::string name, verStr, reasonStr;
            uint64_t txId = 0;
            ss >> name >> verStr >> reasonStr >> txId;
            auto vRes = SemVer::parse(verStr);
            if (vRes.isErr()) continue;

            InstallReason reason = (reasonStr == "dependency") ? InstallReason::DEPENDENCY : InstallReason::EXPLICIT;
            state.recordInstall(InstalledPackage(name, vRes.value(), reason, "", txId));
        }
    }

    return Result<InstalledState>::ok(state);
}

Result<void> Persistence::saveHistory(const HistoryManager& history, const std::string& filePath) {
    std::string tmpPath = filePath + ".tmp";
    std::ofstream out(tmpPath, std::ios::trunc);
    if (!out.is_open()) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Cannot open file for writing: " + tmpPath);
    }

    out << "# HISTORY_LOG_V1\n";
    for (const auto& rec : history.getAll()) {
        out << "TX " << rec.transactionId << " " << transactionStatusToString(rec.status) << " " << rec.actionName << "\n";
        out << "SUMMARY " << rec.summary << "\n";
        out << "PKGS";
        for (const auto& pkg : rec.affectedPackages) out << " " << pkg;
        out << "\nEND_TX\n";
    }

    out.flush();
    out.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Atomic rename failed: " + ec.message());
    }

    return Result<void>::ok();
}

Result<void> Persistence::loadHistory(HistoryManager& history, const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Cannot open file: " + filePath);
    }

    std::string line;
    TransactionRecord cur;
    bool inTx = false;

    while (std::getline(in, line)) {
        line = trimStr(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "TX") {
            inTx = true;
            std::string statusStr;
            ss >> cur.transactionId >> statusStr >> cur.actionName;
            if (statusStr == "COMMITTED") cur.status = TransactionStatus::COMMITTED;
            else if (statusStr == "ROLLED_BACK") cur.status = TransactionStatus::ROLLED_BACK;
            else cur.status = TransactionStatus::ACTIVE;
        } else if (tag == "SUMMARY") {
            std::getline(ss, cur.summary);
            cur.summary = trimStr(cur.summary);
        } else if (tag == "PKGS") {
            std::string p;
            while (ss >> p) cur.affectedPackages.push_back(p);
        } else if (tag == "END_TX") {
            if (inTx) {
                history.append(cur);
                cur = TransactionRecord{};
                inTx = false;
            }
        }
    }

    return Result<void>::ok();
}

Result<void> Persistence::appendJournal(const std::string& journalPath, const std::string& line) {
    std::ofstream out(journalPath, std::ios::app);
    if (!out.is_open()) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Cannot open journal for appending: " + journalPath);
    }
    out << line << "\n";
    out.flush();
    return Result<void>::ok();
}

Result<void> Persistence::clearJournal(const std::string& journalPath) {
    std::ofstream out(journalPath, std::ios::trunc);
    if (!out.is_open()) {
        return Result<void>::err(ErrorCode::IO_ERROR, "Cannot clear journal: " + journalPath);
    }
    out.close();
    return Result<void>::ok();
}

Result<bool> Persistence::performCrashRecovery(const std::string& journalPath, InstalledState& state) {
    if (!std::filesystem::exists(journalPath)) {
        return Result<bool>::ok(false);
    }

    std::ifstream in(journalPath);
    if (!in.is_open()) {
        return Result<bool>::err(ErrorCode::IO_ERROR, "Cannot open journal for crash recovery: " + journalPath);
    }

    std::string line;
    uint64_t uncommittedTx = 0;
    std::vector<std::string> installedInCrashTx;

    while (std::getline(in, line)) {
        line = trimStr(line);
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "TX_BEGIN") {
            ss >> uncommittedTx;
            installedInCrashTx.clear();
        } else if (tag == "TX_INSTALLED") {
            std::string pkg;
            ss >> pkg;
            installedInCrashTx.push_back(pkg);
        } else if (tag == "TX_COMMIT") {
            uncommittedTx = 0;
            installedInCrashTx.clear();
        }
    }

    if (uncommittedTx > 0) {
        // Crash occurred while transaction was ACTIVE!
        // Unwind any partial installs
        for (const auto& pkg : installedInCrashTx) {
            state.recordUninstall(pkg);
        }
        appendJournal(journalPath, "TX_RECOVERED " + std::to_string(uncommittedTx));
        return Result<bool>::ok(true); // Recovery performed
    }

    return Result<bool>::ok(false);
}

} // namespace DepResolver

