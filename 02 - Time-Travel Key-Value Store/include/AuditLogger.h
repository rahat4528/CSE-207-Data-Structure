#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace ttkv {

struct AuditPolicy {
    bool logReads{false};          // False by default to prevent write amplification on reads
    bool logWrites{true};          // True by default
    bool logDeletes{true};         // True by default
    bool logRollbacks{true};       // True by default
    bool logImportsExports{true};  // True by default
    bool logTransactions{true};    // True by default
    bool includePayloadValues{false}; // Metadata only by default
};

struct AuditRecord {
    uint64_t auditId{0};
    uint64_t timestamp{0};
    std::string operation;   // "READ", "WRITE", "DELETE", "ROLLBACK", "BEGIN", "COMMIT", "ABORT", etc.
    std::string key;
    std::string author;
    bool success{true};
    ErrorCode errorCode{ErrorCode::OK};
    std::string details;
};

class AuditLogger {
public:
    explicit AuditLogger(AuditPolicy policy = AuditPolicy{});

    const AuditPolicy& policy() const { return m_policy; }
    void setPolicy(const AuditPolicy& policy) { m_policy = policy; }

    // Logs an operation according to the active policy
    void log(uint64_t timestamp,
             const std::string& op,
             const std::string& key,
             const std::string& author,
             bool success,
             ErrorCode errorCode = ErrorCode::OK,
             const std::string& details = "");

    const std::vector<AuditRecord>& records() const { return m_records; }
    size_t count() const { return m_records.size(); }

    std::vector<AuditRecord> query(std::optional<std::string> opFilter = std::nullopt,
                                  std::optional<std::string> keyFilter = std::nullopt,
                                  std::optional<uint64_t> startTime = std::nullopt,
                                  std::optional<uint64_t> endTime = std::nullopt) const;

    void clear();

private:
    AuditPolicy m_policy;
    std::vector<AuditRecord> m_records;
    uint64_t m_nextAuditId{1};
};

} // namespace ttkv
