# Project 02: Time-Travel Key-Value Store (Versioned Storage Engine)

An enterprise-grade, high-performance time-travel key-value storage engine implemented in ISO C++20 with zero external dependencies.

---

## Features
- **Immutable Versioning:** Every write mutation (`SET`, `UPDATE`, `DELETE`, `ROLLBACK`) appends an immutable `VersionRecord`.
- **Tombstone Deletion:** Deleting a key writes a tombstone version, keeping prior history 100% intact.
- **Point-in-Time Travel:**
  - `GET key` — latest value ($O(1)$)
  - `GET key AT VERSION <n>` — version number lookup ($O(1)$)
  - `GET key AT TIME <ISO-8601>` — temporal lookup via $O(\log V)$ upper-bound binary search
- **State Diffs & Snapshots:** Single-key diff, full store historical diff, and point-in-time store snapshots ($O(K \log V)$).
- **Lightweight Named Snapshots:** $O(1)$ markers protecting baseline versions from retention pruning.
- **ACID Transactions:** `BEGIN`, `COMMIT`, `ABORT` with staged write overlay, read-your-own-writes, and rollback stack undo.
- **Binary Write-Ahead Log (WAL):** Little-endian append-only persistence with IEEE 802.3 CRC32 per record, crash recovery, and torn tail truncation.
- **Concurrent Readers-Writer Lock:** Multi-threaded concurrency with `std::shared_mutex`.
- **JSON & CSV Import/Export:** Human-readable export with dry-run schema validation.

---

## Build & Test Instructions

### Windows (MinGW-w64 GCC)
```cmd
cd "02 - Time-Travel Key-Value Store"
build.bat
```

Run test suite:
```cmd
bin\test_all.exe
```

Run interactive CLI REPL:
```cmd
bin\ttkv.exe
```

Run automated demo walkthrough:
```cmd
bin\ttkv.exe --demo
```

---

## Multi-Language Demos
- Python 3: `python python/time_travel_demo.py`
- Java 17: `java java/TimeTravelDemo.java`
