#include "Common.h"
#include "LeaderboardManager.h"
#include "FriendsManager.h"
#include "AuditManager.h"
#include "CacheAdapter.h"
#include "CLI.h"
#include <iostream>
#include <vector>
#include <memory>

using namespace LeaderboardEngine;

int main(int argc, char* argv[]) {
    auto clock = std::make_shared<SystemClock>();
    auto mgr = std::make_shared<LeaderboardManager>(clock);
    auto friends = std::make_shared<FriendsManager>(mgr);
    auto audit = std::make_shared<AuditManager>(mgr, "ADMIN_SECRET_KEY");
    auto cache = std::make_shared<InMemoryCacheAdapter>(clock);

    CLI cli(mgr, friends, audit, cache);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty()) {
        cli.seedDemoData();
        cli.runRepl();
        return 0;
    }

    return cli.executeCommand(args);
}
