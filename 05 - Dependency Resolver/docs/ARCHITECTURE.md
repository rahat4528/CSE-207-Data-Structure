# Project 05: Dependency Resolver — Architecture Specification

## 1. System Pipeline Architecture

```text
               +-----------------------------------------+
               |        User Interface (CLI / REPL)      |
               +-----------------------------------------+
                                    |
                                    v
               +-----------------------------------------+
               |         Package Manager Engine          |
               +-----------------------------------------+
                                    |
                                    v
               +-----------------------------------------+
               |      Deterministic Backtracking Solver  |
               +-----------------------------------------+
                                    |
                                    v
               +-----------------------------------------+
               |       Directed Acyclic Graph (DAG)      |
               | (Dual-Adjacency: Dependencies & Dependents)|
               +-----------------------------------------+
                     /                             \
                    /                               \
                   v                                 v
    +-----------------------------+   +-----------------------------+
    |   Iterative 3-Color DFS     |   |   Deterministic Kahn's BFS  |
    |      Cycle Detection        |   |      Topological Sort       |
    |  - Exact Loop Extraction    |   |  - Priority Queue Determinism|
    |  - Tarjan's SCC Groups      |   |  - Parallel Execution Layers|
    +-----------------------------+   +-----------------------------+
                   \                                 /
                    \                               /
                     +--------------+--------------+
                                    |
                                    v
               +-----------------------------------------+
               |       Independent Plan Verifier         |
               +-----------------------------------------+
                                    |
                                    v
               +-----------------------------------------+
               |     Simulation & Rollback Engine        |
               |  - 5-Phase Package Lifecycle            |
               |  - LIFO Rollback Stack                  |
               |  - Write-Ahead Crash Journal            |
               +-----------------------------------------+
                                    |
                                    v
               +-----------------------------------------+
               |    Durable Storage & History (List)     |
               +-----------------------------------------+
```

## 2. Core Modules & Responsibilities

| Module | Primary Class / Component | Responsibilities |
|:---|:---|:---|
| **Common** | `Result<T>`, `Diagnostic`, `Validator` | Stable error codes, diagnostic causation chains, package name validation, terminal control character sanitization. |
| **SemVer** | `SemVer` | SemVer 2.0.0 parser, 64-bit numeric comparison, pre-release precedence, build metadata exclusion. |
| **Constraint** | `Constraint`, `VersionInterval` | Caret (`^`), tilde (`~`), wildcards, intervals $[v_{min}, v_{max})$, continuous intersection, conflict detection. |
| **Graph** | `DependencyGraph` | Interned integer `NodeId`, forward adjacency (`u -> v`), reverse adjacency (`v <- u`), transitive reachability. |
| **Registry** | `Registry`, `Package` | Package catalog, multi-version storage, search by name/substring, dependent cascade deletion guard. |
| **Cycle** | `CycleDetector` | Iterative 3-color DFS (White, Gray, Black) on explicit heap stack ($100{,}000+$ depth safe), exact cycle loop extraction, Tarjan SCC. |
| **Topo** | `TopologicalSorter` | Kahn's algorithm (BFS) with min-priority queue, DFS post-order reversal, parallel execution layers, multiple valid orders exploration. |
| **Verifier** | `PlanVerifier` | Independent second-pass plan validation (checks prerequisites, detects duplicate/missing steps, rejects illegal transitions). |
| **Resolver** | `Resolver` | Backtracking constraint propagation solver with forward checking, step limits, and diamond dependency resolution. |
| **State** | `StateManager`, `InstalledState` | Installed package tracking, orphan detection ($O(V + E)$ BFS reachability), unused package discovery, upgrade/downgrade transition warnings. |
| **Simulation** | `SimulationEngine` | 5-step simulated package lifecycle (`FETCH` $\to$ `VERIFY` $\to$ `UNPACK` $\to$ `CONFIGURE` $\to$ `REGISTER`), failure injector, clock simulation. |
| **Transactions** | `TransactionManager` | Atomic execution unit with LIFO rollback stack, complete restoration to pre-transaction state, rollback incomplete detection. |
| **History** | `HistoryManager`, `HistoryNode` | Doubly-linked list for durable transaction history records, package search, and audit trails. |
| **Persistence** | `Persistence` | Atomic file saves (`.tmp` $\to$ rename), strict line-based format parser, write-ahead journal crash recovery. |
| **Exports** | `Exporter` | RFC 4180 CSV with formula injection prevention, RFC 8259 JSON, lockfile (`pkg.lock`) generator, configuration diffing. |
| **Visualizer** | `Visualizer` | Unicode/ASCII tree with cycle and diamond deduplication markers, reverse "why" tree, standalone vector SVG generator. |
| **CLI & REPL** | `CLI` | Subcommands router (`resolve`, `install`, `tree`, `why`, `orphans`, `history`, `doctor`) + interactive console REPL. |
