cmake_minimum_required(VERSION 3.16)
project(TimeTravelStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /WX- /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

include_directories(include)

set(CORE_SOURCES
    src/CRC32.cpp
    src/Timestamp.cpp
    src/Validation.cpp
    src/KeyManager.cpp
    src/TemporalSearch.cpp
    src/HistoryEngine.cpp
    src/SnapshotEngine.cpp
    src/EventLog.cpp
    src/AuditLogger.cpp
    src/StorageEngine.cpp
    src/TransactionManager.cpp
    src/TimeTravelStore.cpp
    src/ExporterImporter.cpp
    src/CLI.cpp
)

set(TEST_SOURCES
    tests/test_all.cpp
    tests/test_stage1_utils.cpp
    tests/test_keys.cpp
    tests/test_versions.cpp
    tests/test_temporal_search.cpp
    tests/test_tombstones.cpp
    tests/test_diff_snapshot.cpp
    tests/test_event_log_audit.cpp
    tests/test_persistence.cpp
    tests/test_transactions.cpp
    tests/test_snapshots_ttl.cpp
    tests/test_concurrency.cpp
    tests/test_import_export.cpp
)

# Core Time-Travel KV Engine CLI
add_executable(ttkv
    src/main.cpp
    ${CORE_SOURCES}
)

# Master Test Runner
add_executable(test_all
    ${TEST_SOURCES}
    ${CORE_SOURCES}
)
