#ifndef AUDIT_MANAGER_H
#define AUDIT_MANAGER_H

#include "Common.h"
#include "LeaderboardManager.h"
#include <string>
#include <vector>
#include <shared_mutex>

namespace LeaderboardEngine {

struct AuditEntry {
    std::string auditId;
    std::string adminId;
    std::string playerId;
    std::string category;
    std::string reason;
    score_t oldScore;
    score_t newScore;
    timestamp_t timestamp;
};

struct AdminCorrectionRequest {
    std::string adminId;
    std::string adminToken;
    std::string playerId;
    std::string category;
    score_t newScore;
    std::string reason;
};

class AuditManager {
public:
    explicit AuditManager(std::shared_ptr<LeaderboardManager> mgr, std::string requiredAdminToken = "ADMIN_SECRET_KEY");

    Result<void> applyCorrection(const AdminCorrectionRequest& req);
    Result<void> banPlayer(const std::string& adminId, const std::string& adminToken, const std::string& playerId, const std::string& reason);

    const std::vector<AuditEntry>& auditLog() const { return auditLog_; }
    size_t auditCount() const;

private:
    std::shared_ptr<LeaderboardManager> mgr_;
    std::string requiredAdminToken_;
    mutable std::shared_mutex auditLock_;
    std::vector<AuditEntry> auditLog_;
};

} // namespace LeaderboardEngine

#endif // AUDIT_MANAGER_H
