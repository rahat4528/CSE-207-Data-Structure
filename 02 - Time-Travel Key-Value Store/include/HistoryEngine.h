#pragma once

#include "Common.h"
#include "DataModel.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ttkv {

struct HistoryEntry {
    uint32_t versionNumber{0};
    uint64_t globalVersionId{0};
    uint64_t timestamp{0};
    std::string formattedTime;
    OpType opType{OpType::OP_SET};
    std::string opString;
    bool isDeleted{false};
    std::string value;
    std::string author;
    std::string description;
};

class HistoryEngine {
public:
    // Retrieves historical records for a key with optional pagination (limit = 0 means no limit)
    static Result<std::vector<HistoryEntry>> getKeyHistory(const KeyRecord& keyRec,
                                                           size_t limit = 0,
                                                           size_t offset = 0);

    // Formats historical entries into a human-readable ASCII table
    static std::string formatHistoryTable(const std::string& key,
                                         const std::vector<HistoryEntry>& entries);
};

} // namespace ttkv
