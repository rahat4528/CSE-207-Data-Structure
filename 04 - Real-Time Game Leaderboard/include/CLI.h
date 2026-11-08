#ifndef CLI_H
#define CLI_H

#include "Common.h"
#include "LeaderboardManager.h"
#include "FriendsManager.h"
#include "AuditManager.h"
#include "CacheAdapter.h"
#include "Dashboard.h"
#include <string>
#include <vector>
#include <memory>

namespace LeaderboardEngine {

class CLI {
public:
    CLI(std::shared_ptr<LeaderboardManager> mgr,
        std::shared_ptr<FriendsManager> friends,
        std::shared_ptr<AuditManager> audit,
        std::shared_ptr<ICacheAdapter> cache);

    int executeCommand(const std::vector<std::string>& args);
    void runRepl();
    void seedDemoData();

private:
    std::shared_ptr<LeaderboardManager> mgr_;
    std::shared_ptr<FriendsManager> friends_;
    std::shared_ptr<AuditManager> audit_;
    std::shared_ptr<ICacheAdapter> cache_;
    TerminalDashboard dashboard_;

    void printHelp() const;
};

} // namespace LeaderboardEngine

#endif // CLI_H
