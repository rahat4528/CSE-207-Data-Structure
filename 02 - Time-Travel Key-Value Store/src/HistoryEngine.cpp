#include "HistoryEngine.h"
#include "Timestamp.h"
#include <sstream>
#include <iomanip>

namespace ttkv {

Result<std::vector<HistoryEntry>> HistoryEngine::getKeyHistory(const KeyRecord& keyRec,
                                                               size_t limit,
                                                               size_t offset) {
    if (keyRec.versions.empty()) {
        return Result<std::vector<HistoryEntry>>::error(ErrorCode::ERR_KEY_UNKNOWN, "No history available");
    }

    std::vector<HistoryEntry> entries;
    size_t total = keyRec.versions.size();

    if (offset >= total) {
        return Result<std::vector<HistoryEntry>>::ok(entries);
    }

    size_t end = (limit == 0) ? total : std::min(total, offset + limit);

    for (size_t i = offset; i < end; ++i) {
        const auto& v = keyRec.versions[i];
        HistoryEntry entry;
        entry.versionNumber = v.keyVersionNumber();
        entry.globalVersionId = v.globalVersionId();
        entry.timestamp = v.timestamp();
        entry.formattedTime = Timestamp::formatIso8601(v.timestamp());
        entry.opType = v.opType();
        entry.opString = opTypeToString(v.opType());
        entry.isDeleted = v.isDeleted();
        entry.value = v.isDeleted() ? "[DELETED]" : v.value();
        entry.author = v.author();
        entry.description = v.description();
        entries.push_back(std::move(entry));
    }

    return Result<std::vector<HistoryEntry>>::ok(entries);
}

std::string HistoryEngine::formatHistoryTable(const std::string& key,
                                              const std::vector<HistoryEntry>& entries) {
    std::ostringstream oss;
    oss << "History for key: '" << key << "' (" << entries.size() << " versions)\n";
    oss << "+---------+--------+--------------------------+----------+----------------------+------------+\n";
    oss << "| Version | Seq ID | Timestamp                | Op       | Value                | Author     |\n";
    oss << "+---------+--------+--------------------------+----------+----------------------+------------+\n";

    for (const auto& e : entries) {
        std::string valDisplay = e.value;
        if (valDisplay.size() > 20) {
            valDisplay = valDisplay.substr(0, 17) + "...";
        }
        oss << "| " << std::setw(7) << e.versionNumber << " | "
            << std::setw(6) << e.globalVersionId << " | "
            << std::setw(24) << e.formattedTime << " | "
            << std::setw(8) << e.opString << " | "
            << std::setw(20) << valDisplay << " | "
            << std::setw(10) << e.author << " |\n";
    }

    oss << "+---------+--------+--------------------------+----------+----------------------+------------+\n";
    return oss.str();
}

} // namespace ttkv
