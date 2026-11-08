#include "AuditLogger.h"

namespace ttkv {

AuditLogger::AuditLogger(AuditPolicy policy) : m_policy(policy) {}

void AuditLogger::log(uint64_t timestamp,
                      const std::string& op,
                      const std::string& key,
                      const std::string& author,
                      bool success,
                      ErrorCode errorCode,
                      const std::string& details) {
    // Check whether the operation should be logged under policy
    if (op == "READ" && !m_policy.logReads) return;
    if ((op == "WRITE" || op == "SET" || op == "UPDATE") && !m_policy.logWrites) return;
    if (op == "DELETE" && !m_policy.logDeletes) return;
    if (op == "ROLLBACK" && !m_policy.logRollbacks) return;
    if ((op == "IMPORT" || op == "EXPORT") && !m_policy.logImportsExports) return;
    if ((op == "BEGIN" || op == "COMMIT" || op == "ABORT") && !m_policy.logTransactions) return;

    AuditRecord record;
    record.auditId = m_nextAuditId++;
    record.timestamp = timestamp;
    record.operation = op;
    record.key = key;
    record.author = author;
    record.success = success;
    record.errorCode = errorCode;
    record.details = details;

    m_records.push_back(std::move(record));
}

std::vector<AuditRecord> AuditLogger::query(std::optional<std::string> opFilter,
                                           std::optional<std::string> keyFilter,
                                           std::optional<uint64_t> startTime,
                                           std::optional<uint64_t> endTime) const {
    std::vector<AuditRecord> result;
    for (const auto& rec : m_records) {
        if (opFilter.has_value() && rec.operation != opFilter.value()) continue;
        if (keyFilter.has_value() && rec.key != keyFilter.value()) continue;
        if (startTime.has_value() && rec.timestamp < startTime.value()) continue;
        if (endTime.has_value() && rec.timestamp > endTime.value()) continue;
        result.push_back(rec);
    }
    return result;
}

void AuditLogger::clear() {
    m_records.clear();
    m_nextAuditId = 1;
}

} // namespace ttkv
