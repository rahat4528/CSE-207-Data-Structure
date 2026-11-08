#include "Persistence.h"
#include <sstream>
#include <iostream>
#include <cstdio>

namespace LeaderboardEngine {

PersistenceManager::PersistenceManager(std::string dataDir, std::shared_ptr<LeaderboardManager> mgr, std::shared_ptr<AuditManager> audit)
    : dataDir_(std::move(dataDir)), mgr_(std::move(mgr)), audit_(std::move(audit)) {
    if (!dataDir_.empty() && dataDir_.back() != '/' && dataDir_.back() != '\\') {
        dataDir_ += "/";
    }
}

std::string PersistenceManager::getWalPath(const std::string& filename) const {
    return dataDir_ + filename;
}

std::string PersistenceManager::getSnapPath(const std::string& filename) const {
    return dataDir_ + filename;
}

void PersistenceManager::ensureWalOpen() {
    if (!walStream_.is_open()) {
        std::string path = getWalPath("events.wal");
        walStream_.open(path, std::ios::out | std::ios::app);
    }
}

Result<void> PersistenceManager::appendPlayerRegistration(const std::string& playerId, const std::string& username, timestamp_t time) {
    ensureWalOpen();
    if (!walStream_.is_open()) {
        return Result<void>::Failure(ErrorCode::STORAGE_ERROR, "Cannot open WAL for write");
    }

    std::ostringstream oss;
    oss << "REGISTER " << playerId << " " << username << " " << time;
    std::string line = oss.str();
    uint32_t crc = calculateCrc32(reinterpret_cast<const uint8_t*>(line.data()), line.size());

    walStream_ << line << " " << crc << "\n";
    walStream_.flush();
    return Result<void>::Success();
}

Result<void> PersistenceManager::appendScoreEvent(const ScoreEvent& event) {
    ensureWalOpen();
    if (!walStream_.is_open()) {
        return Result<void>::Failure(ErrorCode::STORAGE_ERROR, "Cannot open WAL for write");
    }

    std::ostringstream oss;
    oss << "SCORE " << event.eventId << " " << event.playerId << " " << event.category << " "
        << event.value << " " << static_cast<int>(event.mode) << " " << event.timestamp;
    std::string line = oss.str();
    uint32_t crc = calculateCrc32(reinterpret_cast<const uint8_t*>(line.data()), line.size());

    walStream_ << line << " " << crc << "\n";
    walStream_.flush();
    return Result<void>::Success();
}

Result<void> PersistenceManager::appendAuditEntry(const AuditEntry& entry) {
    ensureWalOpen();
    if (!walStream_.is_open()) {
        return Result<void>::Failure(ErrorCode::STORAGE_ERROR, "Cannot open WAL for write");
    }

    std::ostringstream oss;
    oss << "AUDIT " << entry.auditId << " " << entry.adminId << " " << entry.playerId << " "
        << entry.category << " " << entry.oldScore << " " << entry.newScore;
    std::string line = oss.str();
    uint32_t crc = calculateCrc32(reinterpret_cast<const uint8_t*>(line.data()), line.size());

    walStream_ << line << " " << crc << "\n";
    walStream_.flush();
    return Result<void>::Success();
}

Result<void> PersistenceManager::saveSnapshot(const std::string& snapshotFilename) {
    std::string snapPath = getSnapPath(snapshotFilename);
    std::string tmpPath = snapPath + ".tmp";

    std::ofstream out(tmpPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return Result<void>::Failure(ErrorCode::STORAGE_ERROR, "Cannot create temp snapshot file");
    }

    std::ostringstream content;
    content << "SNAPSHOT_V1\n";

    // Write players
    content << "PLAYERS " << mgr_->totalPlayers() << "\n";
    // We snapshot registered players
    // For demo simplicity, snapshot header and content
    std::string body = content.str();
    uint32_t crc = calculateCrc32(reinterpret_cast<const uint8_t*>(body.data()), body.size());

    out << body << "CRC " << crc << "\n";
    out.close();

    // Atomic replace (remove existing target first if on Windows, then rename)
    std::remove(snapPath.c_str());
    if (std::rename(tmpPath.c_str(), snapPath.c_str()) != 0) {
        return Result<void>::Failure(ErrorCode::STORAGE_ERROR, "Failed to rename temp snapshot atomically");
    }

    return Result<void>::Success();
}

Result<size_t> PersistenceManager::recoverFromDisk(const std::string& snapshotFilename, const std::string& walFilename) {
    size_t replayedEvents = 0;
    std::string walPath = getWalPath(walFilename);

    std::ifstream in(walPath);
    if (!in.is_open()) {
        return Result<size_t>::Success(0); // No previous WAL, clean startup
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        // Parse line and checksum
        size_t lastSpace = line.rfind(' ');
        if (lastSpace == std::string::npos) continue;

        std::string payload = line.substr(0, lastSpace);
        std::string crcStr = line.substr(lastSpace + 1);

        uint32_t expectedCrc = 0;
        try {
            expectedCrc = static_cast<uint32_t>(std::stoul(crcStr));
        } catch (...) {
            continue; // Skip corrupted record
        }

        uint32_t actualCrc = calculateCrc32(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
        if (expectedCrc != actualCrc) {
            std::cerr << "[WAL Recovery] CRC mismatch, ignoring corrupted entry.\n";
            continue;
        }

        std::istringstream iss(payload);
        std::string tag;
        iss >> tag;

        if (tag == "REGISTER") {
            std::string pid, uname;
            timestamp_t time;
            if (iss >> pid >> uname >> time) {
                mgr_->registerPlayer(pid, uname);
                replayedEvents++;
            }
        } else if (tag == "SCORE") {
            std::string eid, pid, cat;
            score_t val;
            int modeInt;
            timestamp_t time;
            if (iss >> eid >> pid >> cat >> val >> modeInt >> time) {
                ScoreEvent ev{eid, pid, cat, val, static_cast<ScoreUpdateMode>(modeInt), time, ""};
                mgr_->submitScore(ev);
                replayedEvents++;
            }
        }
    }

    return Result<size_t>::Success(replayedEvents);
}

void PersistenceManager::close() {
    if (walStream_.is_open()) {
        walStream_.close();
    }
}

} // namespace LeaderboardEngine
