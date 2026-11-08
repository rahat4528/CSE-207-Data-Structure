@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   PROJECT 05: DEPENDENCY RESOLVER - BUILD AND TEST
echo   Language: C++17 (g++) - Dual-Adjacency DAG - Kahn BFS
echo =======================================================

where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] g++ not found in PATH!
    exit /b 1
)

if not exist bin mkdir bin

set SRCS=src/SemVer.cpp src/Constraint.cpp src/Package.cpp src/DependencyGraph.cpp src/Registry.cpp src/CycleDetector.cpp src/TopologicalSorter.cpp src/PlanVerifier.cpp src/Resolver.cpp src/StateManager.cpp src/SimulationEngine.cpp src/TransactionManager.cpp src/HistoryManager.cpp src/Persistence.cpp src/Exporter.cpp src/Visualizer.cpp

echo --- Phase 1: Compiling Binaries and Test Suites ---
echo [1/11] Compiling test_common...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests tests/test_common.cpp -o bin/test_common.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [2/11] Compiling test_semver...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests src/SemVer.cpp src/Constraint.cpp tests/test_semver.cpp -o bin/test_semver.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [3/11] Compiling test_graph...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_graph.cpp -o bin/test_graph.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [4/11] Compiling test_cycle...
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_cycle.cpp -o bin/test_cycle.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [5/11] Compiling test_topo...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_topo.cpp -o bin/test_topo.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [6/11] Compiling test_resolver...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_resolver.cpp -o bin/test_resolver.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [7/11] Compiling test_state...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_state.cpp -o bin/test_state.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [8/11] Compiling test_sim...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_simulation.cpp -o bin/test_sim.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [9/11] Compiling test_persistence...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_persistence.cpp -o bin/test_persistence.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [10/11] Compiling test_export...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Itests %SRCS% tests/test_export.cpp -o bin/test_export.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo [11/11] Compiling pkg-mgr CLI application...
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude %SRCS% src/CLI.cpp src/main.cpp -o bin/pkg-mgr.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo.
echo --- Phase 2: Running Automated Test Suites (46 Test Cases) ---
bin\test_common.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_semver.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_graph.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_cycle.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_topo.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_resolver.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_state.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_sim.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_persistence.exe
if %ERRORLEVEL% neq 0 exit /b 1

bin\test_export.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo.
echo =======================================================
echo   ALL 10 TEST SUITES (46 TEST CASES) PASSED (100%%)
echo   Executable ready at: bin\pkg-mgr.exe
echo =======================================================
