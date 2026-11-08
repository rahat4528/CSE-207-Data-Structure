#pragma once

#include "Common.h"
#include "DataModel.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "SnapshotEngine.h"
#include "StorageEngine.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ttkv {

struct PruneResult {
    size_t versionsPruned{0};
    size_t versionsRetained{0};
    size_t protectedBySnapshots{0};
    bool isDryRun{false};
};

class ExporterImporter {
public:
    // JSON & CSV Export
    static std::string exportToJson(const KeyManager& km);
    static std::string exportToCsv(const KeyManager& km);

    // Safe Import with all-or-nothing validation, duplicate ID rejection, and dry-run support
    static Result<size_t> importFromJson(KeyManager& km,
                                         EventLog& el,
                                         StorageEngine* storage,
                                         const std::string& jsonContent,
                                         bool dryRun = false);

    // Retention Pruning protecting named snapshots and keeping baseline versions
    static Result<PruneResult> pruneHistory(KeyManager& km,
                                            const SnapshotEngine& se,
                                            size_t keepLastN = 0,
                                            uint64_t olderThanTime = 0,
                                            bool dryRun = false);
};

} // namespace ttkv
