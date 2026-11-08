# Project 05: Mini Package Manager & Dependency Resolver (C++17)

A complete, production-grade Mini Package Manager in **C++17** demonstrating real-world dependency resolution algorithms: Directed Acyclic Graphs (DAG), Iterative DFS Cycle Detection with exact path extraction, Deterministic Kahn's Algorithm (BFS) Topological Sorting, SemVer 2.0.0 interval math, backtracking constraint solver, failure-injected simulation with LIFO rollback stack, write-ahead crash recovery, and multi-format exports (CSV, JSON, SVG, Lockfile).

Companion demonstration scripts in **Python** (secondary) and **Java** (tertiary) are included in `python/` and `java/`.

---

## 🔬 Core Features & Data Structures

| Feature | Data Structure / Algorithm | File Location |
|:---|:---|:---|
| **Dual-Adjacency Graph** | Forward & Reverse Adjacency Lists ($O(1)$ lookup) | `include/DependencyGraph.h`, `src/DependencyGraph.cpp` |
| **Cycle Detection** | Iterative 3-State DFS with Explicit Stack + Tarjan SCC | `include/CycleDetector.h`, `src/CycleDetector.cpp` |
| **Topological Sorting** | Kahn's Algorithm (BFS) with Min-Priority Queue | `include/TopologicalSorter.h`, `src/TopologicalSorter.cpp` |
| **Independent Verification**| Positional Hash Map ($O(V + E)$) | `include/PlanVerifier.h`, `src/PlanVerifier.cpp` |
| **Constraint Solver** | Backtracking DFS with Forward Checking & Pruning | `include/Resolver.h`, `src/Resolver.cpp` |
| **SemVer 2.0.0 Engine** | Precedence Comparator & Version Intervals $[v_{min}, v_{max})$ | `include/SemVer.h`, `src/Constraint.cpp` |
| **Orphan Analysis** | Root-Reachability Traversal | `include/StateManager.h`, `src/StateManager.cpp` |
| **Atomic Rollback** | LIFO Stack (`std::vector<RollbackOp>`) | `include/TransactionManager.h`, `src/TransactionManager.cpp` |
| **Transaction History** | Custom Doubly-Linked List (`HistoryNode`) | `include/HistoryManager.h`, `src/HistoryManager.cpp` |
| **Crash Recovery** | Write-Ahead Transaction Journal | `include/Persistence.h`, `src/Persistence.cpp` |
| **Reporting & Export** | RFC 4180 CSV (Formula Neutralized) & JSON & Lockfile | `include/Exporter.h`, `src/Exporter.cpp` |
| **Visualization** | ASCII Tree with deduplication & Standalone Vector SVG | `include/Visualizer.h`, `src/Visualizer.cpp` |

---

## 🚀 How to Build & Run

### 1. Build & Run All Tests (One-Click Batch Script):
```cmd
build.bat
```
Runs Phase 1 (compilation) and Phase 2 (all 10 test suites, 46 test cases).

### 2. Run the Main Application CLI:
```cmd
# Display all commands
bin\pkg-mgr.exe help

# Resolve dependencies and output topological installation plan
bin\pkg-mgr.exe resolve nextjs

# Simulate installation with atomic transactions & execution logs
bin\pkg-mgr.exe install express

# Render ASCII dependency tree
bin\pkg-mgr.exe tree nextjs

# Trace reverse dependency ("why is package X installed?")
bin\pkg-mgr.exe why cookie-parser

# Generate standalone SVG graph viewable in any browser
bin\pkg-mgr.exe svg dependency_graph.svg

# Detect and remove orphaned packages
bin\pkg-mgr.exe orphans
bin\pkg-mgr.exe autoremove

# Check system health & consistency
bin\pkg-mgr.exe doctor

# Launch Interactive Console REPL
bin\pkg-mgr.exe repl
```

---

## 🐍 Secondary & Tertiary Language Demos

* **Python Secondary Demo:**
  ```cmd
  python python/resolver_demo.py
  ```
* **Java Tertiary Demo:**
  ```cmd
  cd java
  javac DependencyResolverDemo.java
  java DependencyResolverDemo
  ```

---

## 📜 Academic Coursework Note
Developed for **CSE 207: Data Structures** at **East West University**. Fully preserved alongside existing lab experiments and mini-projects.
