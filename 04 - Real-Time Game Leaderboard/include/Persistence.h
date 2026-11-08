#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "Common.h"
#include "LeaderboardManager.h"
#include "AuditManager.h"
#include <string>
#include <fstream>
#include <memory>

namespace LeaderboardEngine {

class PersistenceManager {
public:
    PersistenceManager(std::string dataDir, std::shared_ptr<LeaderboardManager> mgr, std::shared_ptr<AuditManager> audit);

    Result<void> appendPlayerRegistration(const std::string& playerId, const std::string& username, timestamp_t time);
    Result<void> appendScoreEvent(const ScoreEvent& event);
    Result<void> appendAuditEntry(const AuditEntry& entry);

    // Creates an atomic snapshot (temporary file write + atomic rename)
    Result<void> saveSnapshot(const std::string& snapshotFilename = "leaderboard.snap");

    // Recovers state on startup: loads snapshot + replays WAL entries
    Result<size_t> recoverFromDisk(const std::string& snapshotFilename = "leaderboard.snap", const std::string& walFilename = "events.wal");

    const std::string& dataDir() const { return dataDir_; }
    void close();

private:
    std::string dataDir_;
    std::shared_ptr<LeaderboardManager> mgr_;
    std::shared_ptr<AuditManager> audit_;
    std::ofstream walStream_;

    std::string getWalPath(const std::string& filename) const;
    std::string getSnapPath(const std::string& filename) const;
    void ensureWalOpen();
};

} // namespace LeaderboardEngine

#endif // PERSISTENCE_H
