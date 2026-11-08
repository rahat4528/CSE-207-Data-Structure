# Project 04: Real-Time Game Leaderboard (Ranking Engine)

An industrial-grade, in-memory competitive game ranking engine developed in **modern C++17** for the **East West University CSE 207: Data Structures** curriculum.

---

## 🚀 Key Features

* **Exact Competition Ranking ("1224" Rule)**: Uses an in-memory **Fenwick Tree (Binary Indexed Tree)** over a dynamically compressed score domain to answer rank queries in $O(\log S)$ time.
* **Top-K Player Retrieval ($O(1) \dots O(K \log K)$)**: Custom **Indexed Max Heap** with an internal bidirectional position map (`player_id -> heap_slot`) enabling $O(1)$ lookups and $O(\log N)$ updates.
* **Dynamic Coordinate Compression**: Discretizes arbitrary 64-bit continuous scores using a sorted pending buffer and amortized rebuilds.
* **Temporal Partitioning**: Automated time-window rollovers (Daily, Weekly, Monthly, All-Time) with deterministic boundary testing.
* **Seasonal Resets & Archives**: Soft/hard seasonal standings archiving preserving historical player performance.
* **Multi-Threaded Concurrency**: High-throughput read-write synchronization (`std::shared_mutex`) per board, supporting concurrent readers with atomic writers.
* **Social & Administrative Features**: Friends-only leaderboards ($O(F \log F)$), head-to-head comparisons, and tamper-evident administrative score corrections.
* **Zero-Data-Loss Persistence**: Write-Ahead Log (WAL) with CRC32 integrity checksums and atomic snapshot state recovery.
* **Real-Time Live Dashboard**: Full ANSI terminal UI displaying live leaderboards, throughput, and system health in C++.

---

## 🛠️ Build and Test

### 1-Click Native Build (Windows MinGW / GCC)
```cmd
cd "04 - Real-Time Game Leaderboard"
.\build.bat
```

### CMake Build
```cmd
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

---

## 💻 CLI & Interactive REPL Usage

Launch the interactive REPL shell:
```cmd
bin\leaderboard-engine.exe repl
```

Execute direct subcommands:
```cmd
bin\leaderboard-engine.exe seed
bin\leaderboard-engine.exe top 10 arcade
bin\leaderboard-engine.exe player p1 arcade
bin\leaderboard-engine.exe friends p1 arcade
bin\leaderboard-engine.exe compare p1 p2 arcade
bin\leaderboard-engine.exe health
```

---

## 🐍 Python & ☕ Java Companion Demonstrations
```cmd
python python/leaderboard_demo.py
javac java/LeaderboardDemo.java && java -cp java LeaderboardDemo
```
