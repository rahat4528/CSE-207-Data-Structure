cmake_minimum_required(VERSION 3.16)
project(DependencyResolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CORE_SOURCES
    src/SemVer.cpp
    src/Constraint.cpp
    src/Package.cpp
    src/DependencyGraph.cpp
    src/Registry.cpp
    src/CycleDetector.cpp
    src/TopologicalSorter.cpp
    src/PlanVerifier.cpp
    src/Resolver.cpp
    src/StateManager.cpp
    src/SimulationEngine.cpp
    src/TransactionManager.cpp
    src/HistoryManager.cpp
    src/Persistence.cpp
    src/Exporter.cpp
    src/Visualizer.cpp
)

# Main Executable
add_executable(pkg-mgr ${CORE_SOURCES} src/CLI.cpp src/main.cpp)

# Test Executables
add_executable(test_common tests/test_common.cpp)
add_executable(test_semver ${CORE_SOURCES} tests/test_semver.cpp)
add_executable(test_graph ${CORE_SOURCES} tests/test_graph.cpp)
add_executable(test_cycle ${CORE_SOURCES} tests/test_cycle.cpp)
add_executable(test_topo ${CORE_SOURCES} tests/test_topo.cpp)
add_executable(test_resolver ${CORE_SOURCES} tests/test_resolver.cpp)
add_executable(test_state ${CORE_SOURCES} tests/test_state.cpp)
add_executable(test_simulation ${CORE_SOURCES} tests/test_simulation.cpp)
add_executable(test_persistence ${CORE_SOURCES} tests/test_persistence.cpp)
add_executable(test_export ${CORE_SOURCES} tests/test_export.cpp)
