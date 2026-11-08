@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   Building Time-Travel Key-Value Store (ISO C++20)
echo ============================================================

set CXX=g++
set CXXFLAGS=-std=c++20 -O2 -Wall -Wextra -Wpedantic -Wconversion -Iinclude

if not exist bin mkdir bin
if not exist data mkdir data

set CORE_SRCS=src/CRC32.cpp src/Timestamp.cpp src/Validation.cpp src/KeyManager.cpp src/TemporalSearch.cpp src/HistoryEngine.cpp src/SnapshotEngine.cpp src/EventLog.cpp src/AuditLogger.cpp src/StorageEngine.cpp src/TransactionManager.cpp src/TimeTravelStore.cpp src/ExporterImporter.cpp src/CLI.cpp

set TEST_SRCS=tests/test_all.cpp tests/test_stage1_utils.cpp tests/test_keys.cpp tests/test_versions.cpp tests/test_temporal_search.cpp tests/test_tombstones.cpp tests/test_diff_snapshot.cpp tests/test_event_log_audit.cpp tests/test_persistence.cpp tests/test_transactions.cpp tests/test_snapshots_ttl.cpp tests/test_concurrency.cpp tests/test_import_export.cpp

echo Compiling CLI binary (bin/ttkv.exe)...
%CXX% %CXXFLAGS% src/main.cpp %CORE_SRCS% -o bin/ttkv.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CLI build failed!
    exit /b 1
)

echo Compiling Master Test Runner (bin/test_all.exe)...
%CXX% %CXXFLAGS% %TEST_SRCS% %CORE_SRCS% -o bin/test_all.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Test runner build failed!
    exit /b 1
)

echo [SUCCESS] Build completed successfully:
echo   - bin/ttkv.exe
echo   - bin/test_all.exe
