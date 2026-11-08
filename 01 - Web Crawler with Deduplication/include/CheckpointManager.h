#ifndef CRAWLER_CHECKPOINT_MANAGER_H
#define CRAWLER_CHECKPOINT_MANAGER_H

#include "Common.h"
#include "CrawlerEngine.h"
#include <string>

namespace crawler {

class CheckpointManager {
public:
    static Result<void> save(const std::string& filePath, const CrawlerEngine& engine);
    static Result<void> load(const std::string& filePath, CrawlerEngine& engine);

    static constexpr uint32_t MAGIC = 0x4C575243; // 'CRWL'
    static constexpr uint16_t VERSION = 1;
};

} // namespace crawler

#endif // CRAWLER_CHECKPOINT_MANAGER_H

