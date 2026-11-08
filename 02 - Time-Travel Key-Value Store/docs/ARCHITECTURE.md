# Time-Travel Key-Value Store: Architecture & Invariants

## 1. System Mission & Philosophy
The **Time-Travel Key-Value Store** is an enterprise-grade versioned storage engine developed in pure ISO C++20. Unlike conventional databases that mutate records in place, this engine implements an **immutable, append-only data model**:
- Every write mutation (`SET`, `UPDATE`, `DELETE`, `ROLLBACK`, `EXPIRE`) creates an immutable `VersionRecord`.
- History is strictly monotonically ordered by `(timestamp ASC, globalVersionId ASC)`.
- Deletions append **tombstone records**, preserving full historical queryability.
- Queries achieve point-in-time travel to any previous version or timestamp.

---

## 2. Layered Architecture

```text
               USER INTERFACE / API
                         │
                 COMMAND HANDLER (CLI)
                         │
                  TIME TRAVEL STORE
                         │ (std::shared_mutex)
                  KEY MANAGER
                         │
            ┌────────────┴────────────┐
       HASH MAP                  VERSION LIST
  (Key to KeyRecord)         (Per-Key Versions)
            │                         │
      EVENT LOG                 BINARY SEARCH
(Source of Truth)           (O(log V) Temporal)
            │                         │
     AUDIT HISTORY           HISTORICAL ENGINE
(Configurable Audit)         (Point-in-Time & Diff)
            │                         │
            └────────────┬────────────┘
                  STORAGE ENGINE
                         │
            BINARY WAL & ATOMIC SNAPSHOTS
```

---

## 3. Core Data Structure Responsibilities

| Data Structure | Implementation | Algorithmic Responsibility | Time Complexity |
|---|---|---|---|
| **HashMap** | `std::unordered_map<std::string, KeyRecord>` | Instant key location by string name | $O(1)$ average |
| **Version List** | `std::vector<VersionRecord>` | Sequential, contiguous version histories per key | $O(1)$ amortized append |
| **Binary Search** | Custom `binarySearchUpperBound` | Temporal lookup of version active at timestamp $t$ | $O(\log V)$ |
| **Version Chain** | `prevVersionId` linkage | Integrity verification ensuring no records were skipped or reordered | $O(1)$ verification |
| **Rollback Stack** | `std::stack<TxUndoEntry>` | Transaction staged write undo and abort restoration | $O(U)$ where $U$ is undo operations |
| **HashSet** | `std::unordered_set<uint64_t>` | Duplicate version ID rejection during live writes, WAL recovery, and import | $O(1)$ average |
| **Event Log** | Sequential `EventRecord` vector | In-memory append-only log and source of truth for full state reconstruction | $O(1)$ append, $O(N)$ replay |

---

## 4. Query Result Semantics & Decision Flow

For any query at target time $t$ on key $K$:
1. If $K$ is not registered in the KeyManager $\to$ `KEY_UNKNOWN`.
2. Upper-bound binary search locates rightmost version with `timestamp <= t`.
3. If no version exists with `timestamp <= t` (i.e. $t < \text{firstVersion.timestamp}$) $\to$ `NOT_YET_EXISTS`.
4. If version has `isDeleted == true` $\to$ `DELETED`.
5. If version has `expiryTimestamp > 0` and $t \ge \text{expiryTimestamp}$ $\to$ `EXPIRED`.
6. Else $\to$ `FOUND` (returns stored value, version number, sequence ID, author, and description).
