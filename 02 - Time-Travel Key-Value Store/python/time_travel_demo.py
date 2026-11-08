#!/usr/bin/env python3
"""
Time-Travel Key-Value Store — Companion Python Demonstration
Demonstrates immutable version records, upper-bound binary search,
tombstones, and rollback as a new forward version.
"""

from typing import List, Optional, Tuple
import datetime

class VersionRecord:
    def __init__(self, global_id: int, key_ver: int, key: str, value: str,
                 is_deleted: bool, timestamp: int):
        self.global_id = global_id
        self.key_ver = key_ver
        self.key = key
        self.value = value
        self.is_deleted = is_deleted
        self.timestamp = timestamp

    def __repr__(self):
        val_str = "[DELETED]" if self.is_deleted else f"'{self.value}'"
        return f"<Version {self.key_ver} (Seq {self.global_id}) at {self.timestamp}us: {val_str}>"

class TimeTravelStoreDemo:
    def __init__(self):
        self.store = {} # key -> List[VersionRecord]
        self.next_global_id = 1
        self.current_time_micros = 1000000

    def set(self, key: str, value: str) -> int:
        self.current_time_micros += 1000
        g_id = self.next_global_id
        self.next_global_id += 1

        history = self.store.setdefault(key, [])
        k_ver = len(history) + 1
        rec = VersionRecord(g_id, k_ver, key, value, False, self.current_time_micros)
        history.append(rec)
        return g_id

    def delete(self, key: str) -> int:
        self.current_time_micros += 1000
        g_id = self.next_global_id
        self.next_global_id += 1

        history = self.store.setdefault(key, [])
        k_ver = len(history) + 1
        rec = VersionRecord(g_id, k_ver, key, "", True, self.current_time_micros)
        history.append(rec)
        return g_id

    def get_current(self, key: str) -> Optional[str]:
        history = self.store.get(key, [])
        if not history or history[-1].is_deleted:
            return None
        return history[-1].value

    def get_at_version(self, key: str, version: int) -> Optional[str]:
        history = self.store.get(key, [])
        if 1 <= version <= len(history):
            rec = history[version - 1]
            return "[DELETED]" if rec.is_deleted else rec.value
        return None

    def get_at_time(self, key: str, target_time: int) -> Optional[str]:
        history = self.store.get(key, [])
        if not history or target_time < history[0].timestamp:
            return None

        # Binary search upper bound: rightmost element <= target_time
        low, high = 0, len(history) - 1
        best = -1
        while low <= high:
            mid = (low + high) // 2
            if history[mid].timestamp <= target_time:
                best = mid
                low = mid + 1
            else:
                high = mid - 1

        if best >= 0:
            rec = history[best]
            return "[DELETED]" if rec.is_deleted else rec.value
        return None

    def rollback(self, key: str, target_version: int) -> int:
        old_val = self.get_at_version(key, target_version)
        if old_val is None:
            raise ValueError(f"Version {target_version} does not exist.")
        if old_val == "[DELETED]":
            return self.delete(key)
        return self.set(key, old_val)

def main():
    print("=" * 60)
    print("  TIME-TRAVEL KEY-VALUE STORE: PYTHON DEMONSTRATION")
    print("=" * 60)

    store = TimeTravelStoreDemo()

    # Blueprint sequence:
    print("\n1. Appending versions for key 'username':")
    store.set("username", "Alice")      # Version 1
    t1 = store.current_time_micros
    print("   Version 1 set to 'Alice'")

    store.set("username", "Bob")        # Version 2
    t2 = store.current_time_micros
    print("   Version 2 updated to 'Bob'")

    store.set("username", "Charlie")    # Version 3
    t3 = store.current_time_micros
    print("   Version 3 updated to 'Charlie'")

    store.delete("username")            # Version 4: Tombstone
    t4 = store.current_time_micros
    print("   Version 4 deleted (tombstone created)")

    print("\n2. Querying by Version Number:")
    print("   GET username AT VERSION 1 ->", store.get_at_version("username", 1))
    print("   GET username AT VERSION 2 ->", store.get_at_version("username", 2))
    print("   GET username AT VERSION 3 ->", store.get_at_version("username", 3))
    print("   GET username AT VERSION 4 ->", store.get_at_version("username", 4))
    print("   GET username (current)   ->", store.get_current("username"))

    print("\n3. Querying by Historical Timestamp (Binary Search):")
    print(f"   At t1 ({t1} us) ->", store.get_at_time("username", t1))
    print(f"   At t2 ({t2} us) ->", store.get_at_time("username", t2))
    print(f"   At t3 ({t3} us) ->", store.get_at_time("username", t3))
    print(f"   At t4 ({t4} us) ->", store.get_at_time("username", t4))

    print("\n4. Executing Rollback to Version 2 ('Bob'):")
    store.rollback("username", 2)
    print("   GET username (current now) ->", store.get_current("username"))
    print("   GET username AT VERSION 5 ->", store.get_at_version("username", 5))

    print("\n" + "=" * 60)
    print("  PYTHON DEMO COMPLETED SUCCESSFULLY")
    print("=" * 60)

if __name__ == "__main__":
    main()
