@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   PROJECT 04: REAL-TIME GAME LEADERBOARD - BUILD AND TEST
echo   Language: C++17 (g++) - Fenwick Tree - Indexed Max Heap
echo =======================================================

if not exist bin mkdir bin

set CXX=g++
set CXXFLAGS=-std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests

set SOURCES=src/CoordinateCompressor.cpp src/FenwickTree.cpp src/IndexedMaxHeap.cpp src/Player.cpp src/Leaderboard.cpp src/LeaderboardManager.cpp src/FriendsManager.cpp src/AuditManager.cpp src/Persistence.cpp src/CacheAdapter.cpp src/WebSocketBroadcaster.cpp src/RateLimiter.cpp src/Dashboard.cpp src/CLI.cpp

set TEST_SOURCES=tests/test_all.cpp tests/test_fenwick.cpp tests/test_heap.cpp tests/test_ranking.cpp tests/test_windows.cpp tests/test_concurrency.cpp tests/test_persistence.cpp tests/test_cache_websocket.cpp tests/test_load.cpp

echo --- Phase 1: Compiling Leaderboard and Test Suites ---

echo [1/1] Compiling test_all suite and CLI engine...
%CXX% %CXXFLAGS% %TEST_SOURCES% %SOURCES% -o bin/test_all.exe
if %ERRORLEVEL% neq 0 ( echo Error compiling test_all && exit /b 1 )

echo.
echo --- Phase 2: Running Automated Test Suites (21 Test Cases) ---
powershell -Command "Start-Sleep -Milliseconds 500; & 'bin/test_all.exe'"
if %ERRORLEVEL% neq 0 ( echo Automated test suite failed && exit /b 1 )

echo.
echo =======================================================
echo   ALL 21 LEADERBOARD TEST CASES PASSED (100%%)
echo   Application and Tests ready at: bin\test_all.exe
echo =======================================================
exit /b 0
