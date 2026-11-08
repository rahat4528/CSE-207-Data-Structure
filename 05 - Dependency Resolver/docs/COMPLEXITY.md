# Project 05: Dependency Resolver — Complexity & Performance Analysis

## 1. Time & Space Complexity Matrix

| Operation / Algorithm | Implementation Structure | Time Complexity (Best / Worst) | Auxiliary Space | Notes & Guarantees |
|:---|:---|:---:|:---:|:---|
| **SemVer Parsing** | Custom tokenizer | $O(L)$ | $O(L)$ | $L$ = length of version string. 64-bit bounds check. |
| **SemVer Comparison** | Numeric + Lexical compare | $O(P)$ | $O(1)$ | $P$ = count of pre-release segments. Build metadata ignored. |
| **Constraint Intersection** | Interval algebra | $O(I_1 \times I_2)$ | $O(I_1 \times I_2)$ | $I$ = number of OR intervals (typically $1 \le I \le 3$). |
| **Graph Node Interning** | Hash table lookup | $O(1)$ amortized | $O(V)$ | Maps package name $\leftrightarrow$ 32-bit `NodeId`. |
| **Transitive Reachability** | BFS queue | $O(V + E)$ | $O(V)$ | Used for transitive dependencies and dependents ("why"). |
| **Cycle Detection (DFS)** | Explicit heap stack | $O(V + E)$ | $O(V)$ | Iterative 3-color. Safe on $100{,}000+$ chain (0 stack overflow). |
| **Dynamic Edge Cycle Check** | Directed reachability | $O(V + E)$ | $O(V)$ | Verifies if $v$ can reach $u$ before adding $u \to v$. |
| **Tarjan's SCC** | Lowlink DFS stack | $O(V + E)$ | $O(V)$ | Identifies all cyclic groups in single linear pass. |
| **Topological Sort (Kahn BFS)**| Min-Priority Queue | $O((V + E) \log V)$ | $O(V)$ | Lexicographical tie-breaking guarantees 100% determinism. |
| **Topological Sort (DFS)** | Post-order reversal | $O(V + E)$ | $O(V)$ | Used for secondary validation cross-check. |
| **Plan Verification** | Positional hash map | $O(V + E)$ | $O(V)$ | Independent pass asserting prerequisites precede dependents. |
| **Orphan Package Analysis** | Root-reachability BFS | $O(V + E)$ | $O(V)$ | Unreachable packages with `reason == DEPENDENCY`. |
| **Unused Package Analysis** | Reachability set | $O(V + E)$ | $O(V)$ | Registered packages neither installed nor required. |
| **Dependency Resolution** | Backtracking DFS solver | $O(C^V)$ worst-case | $O(V + D)$ | $C$ = versions per package. Forward checking & pruning keep practical cases $< 1$ ms. Guarded by configurable step limit ($50{,}000$). |
| **Simulation & Rollback** | LIFO Rollback Stack | $O(K)$ | $O(K)$ | $K$ = steps completed before failure. Reverses state byte-for-byte. |
| **Persistence (Save/Load)** | Line-based I/O | $O(V + E)$ | $O(V + E)$ | Atomic write via `.tmp` file and rename. |

## 2. Empirical Benchmark Measurements

*Measured on AMD64 Windows 11 using Brecht Sanders MinGW-w64 GCC 16.1.0 (-O2):*

* **Deep Chain Cycle Detection (100,000 Nodes):** $186.18\text{ ms}$ (zero call-stack overflow).
* **Diamond Dependency Constraint Resolution:** $0.41\text{ ms}$ (3 solver steps).
* **Backtracking Resolution to Older Version:** $0.29\text{ ms}$ (4 solver steps).
* **Kahn's BFS Topological Sorting (Standard DAG):** $0.14\text{ ms}$.
* **Simulation Lifecycle & Rollback Execution:** $0.09\text{ ms}$.
* **Registry Atomic Save and Reload Round-trip:** $3.49\text{ ms}$.
