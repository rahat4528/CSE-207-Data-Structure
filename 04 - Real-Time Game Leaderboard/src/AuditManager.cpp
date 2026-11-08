#include "AuditManager.h"

namespace LeaderboardEngine {

AuditManager::AuditManager(std::shared_ptr<LeaderboardManager> mgr, std::string requiredAdminToken)
    : mgr_(std::move(mgr)), requiredAdminToken_(std::move(requiredAdminToken)) {}

Result<void> AuditManager::applyCorrection(const AdminCorrectionRequest& req) {
    if (req.adminToken != requiredAdminToken_) {
        return Result<void>::Failure(ErrorCode::UNAUTHORIZED, "Invalid admin credentials");
    }

    if (req.reason.empty()) {
        return Result<void>::Failure(ErrorCode::INVALID_SCORE, "Reason is mandatory for score corrections");
    }

    auto pRes = mgr_->getPlayer(req.playerId);
    if (!pRes.ok) {
        return Result<void>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not found");
    }

    score_t oldScore = pRes.value.currentScore();

    // Generate unique audit event
    std::string eventId = "audit_corr_" + std::to_string(auditLog_.size() + 1);
    ScoreEvent ev{eventId, req.playerId, req.category, req.newScore, ScoreUpdateMode::SET, 0, "Admin correction: " + req.reason};

    auto submitRes = mgr_->submitScore(ev);
    if (!submitRes.ok) {
        return Result<void>::Failure(submitRes.error, submitRes.message);
    }

    {
        std::unique_lock<std::shared_mutex> lock(auditLock_);
        AuditEntry entry{
            "AUDIT-" + std::to_string(auditLog_.size() + 1),
            req.adminId,
            req.playerId,
            req.category,
            req.reason,
            oldScore,
            req.newScore,
            0
        };
        auditLog_.push_back(std::move(entry));
    }

    return Result<void>::Success();
}

Result<void> AuditManager::banPlayer(const std::string& adminId, const std::string& adminToken, const std::string& playerId, const std::string& reason) {
    if (adminToken != requiredAdminToken_) {
        return Result<void>::Failure(ErrorCode::UNAUTHORIZED, "Invalid admin credentials");
    }

    auto pRes = mgr_->getPlayer(playerId);
    if (!pRes.ok) {
        return Result<void>::Failure(ErrorCode::PLAYER_NOT_FOUND, "Player not found");
    }

    score_t oldScore = pRes.value.currentScore();

    // Set score to 0 or remove from active board
    ScoreEvent ev{"ban_" + playerId, playerId, "all", 0, ScoreUpdateMode::SET, 0, "Banned: " + reason};
    // Mark in audit log
    {
        std::unique_lock<std::shared_mutex> lock(auditLock_);
        AuditEntry entry{
            "BAN-" + std::to_string(auditLog_.size() + 1),
            adminId,
            playerId,
            "ALL",
            "BAN: " + reason,
            oldScore,
            0,
            0
        };
        auditLog_.push_back(std::move(entry));
    }

    return Result<void>::Success();
}

size_t AuditManager::auditCount() const {
    std::shared_lock<std::shared_mutex> lock(auditLock_);
    return auditLog_.size();
}

} // namespace LeaderboardEngine
