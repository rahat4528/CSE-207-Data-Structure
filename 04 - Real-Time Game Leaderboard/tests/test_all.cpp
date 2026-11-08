#include "TestRunner.h"
#include "LeaderboardManager.h"
#include "FriendsManager.h"
#include "AuditManager.h"
#include "CacheAdapter.h"
#include "CLI.h"
#include <vector>

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        return MiniTest::runAllTests();
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.size() == 1 && args[0] == "--test") {
        return MiniTest::runAllTests();
    }

    auto clock = std::make_shared<LeaderboardEngine::SystemClock>();
    auto mgr = std::make_shared<LeaderboardEngine::LeaderboardManager>(clock);
    auto friends = std::make_shared<LeaderboardEngine::FriendsManager>(mgr);
    auto audit = std::make_shared<LeaderboardEngine::AuditManager>(mgr, "ADMIN_SECRET_KEY");
    auto cache = std::make_shared<LeaderboardEngine::InMemoryCacheAdapter>(clock);

    LeaderboardEngine::CLI cli(mgr, friends, audit, cache);
    return cli.executeCommand(args);
}
