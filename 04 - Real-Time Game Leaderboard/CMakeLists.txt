cmake_minimum_required(VERSION 3.15)
project(RealTimeLeaderboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(LIB_SOURCES
    src/CoordinateCompressor.cpp
    src/FenwickTree.cpp
    src/IndexedMaxHeap.cpp
    src/Player.cpp
    src/Leaderboard.cpp
    src/LeaderboardManager.cpp
    src/FriendsManager.cpp
    src/AuditManager.cpp
    src/Persistence.cpp
    src/CacheAdapter.cpp
    src/WebSocketBroadcaster.cpp
    src/RateLimiter.cpp
    src/Dashboard.cpp
    src/CLI.cpp
)

add_executable(leaderboard-app src/main.cpp ${LIB_SOURCES})

enable_testing()
add_executable(test_all
    tests/test_all.cpp
    tests/test_fenwick.cpp
    tests/test_heap.cpp
    tests/test_ranking.cpp
    tests/test_windows.cpp
    tests/test_concurrency.cpp
    tests/test_persistence.cpp
    tests/test_cache_websocket.cpp
    tests/test_load.cpp
    ${LIB_SOURCES}
)
add_test(NAME AllLeaderboardTests COMMAND test_all)
