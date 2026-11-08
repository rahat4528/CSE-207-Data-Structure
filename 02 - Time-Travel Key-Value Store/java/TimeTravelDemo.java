import java.util.*;

/**
 * Time-Travel Key-Value Store — Companion Java Demonstration
 * Demonstrates immutable records, binary search, tombstones, and rollbacks.
 */
public class TimeTravelDemo {

    public static class VersionRecord {
        final long globalVersionId;
        final int keyVersionNumber;
        final String key;
        final String value;
        final boolean isDeleted;
        final long timestamp;

        public VersionRecord(long gId, int kVer, String key, String value, boolean isDeleted, long timestamp) {
            this.globalVersionId = gId;
            this.keyVersionNumber = kVer;
            this.key = key;
            this.value = value;
            this.isDeleted = isDeleted;
            this.timestamp = timestamp;
        }
    }

    private final Map<String, List<VersionRecord>> store = new HashMap<>();
    private long nextGlobalId = 1;
    private long currentTimeMicros = 1000000;

    public synchronized long set(String key, String value) {
        currentTimeMicros += 1000;
        long gId = nextGlobalId++;
        List<VersionRecord> history = store.computeIfAbsent(key, k -> new ArrayList<>());
        int kVer = history.size() + 1;
        VersionRecord rec = new VersionRecord(gId, kVer, key, value, false, currentTimeMicros);
        history.add(rec);
        return gId;
    }

    public synchronized long delete(String key) {
        currentTimeMicros += 1000;
        long gId = nextGlobalId++;
        List<VersionRecord> history = store.computeIfAbsent(key, k -> new ArrayList<>());
        int kVer = history.size() + 1;
        VersionRecord rec = new VersionRecord(gId, kVer, key, "", true, currentTimeMicros);
        history.add(rec);
        return gId;
    }

    public synchronized String getCurrent(String key) {
        List<VersionRecord> history = store.get(key);
        if (history == null || history.isEmpty()) return null;
        VersionRecord latest = history.get(history.size() - 1);
        return latest.isDeleted ? null : latest.value;
    }

    public synchronized String getAtVersion(String key, int version) {
        List<VersionRecord> history = store.get(key);
        if (history == null || version < 1 || version > history.size()) return null;
        VersionRecord rec = history.get(version - 1);
        return rec.isDeleted ? "[DELETED]" : rec.value;
    }

    public synchronized String getAtTime(String key, long targetTime) {
        List<VersionRecord> history = store.get(key);
        if (history == null || history.isEmpty() || targetTime < history.get(0).timestamp) {
            return null;
        }

        // Binary search upper-bound: rightmost <= targetTime
        int low = 0, high = history.size() - 1;
        int best = -1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (history.get(mid).timestamp <= targetTime) {
                best = mid;
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        if (best >= 0) {
            VersionRecord rec = history.get(best);
            return rec.isDeleted ? "[DELETED]" : rec.value;
        }
        return null;
    }

    public synchronized long rollback(String key, int targetVersion) {
        String val = getAtVersion(key, targetVersion);
        if (val == null) throw new IllegalArgumentException("Version not found");
        if ("[DELETED]".equals(val)) return delete(key);
        return set(key, val);
    }

    public static void main(String[] args) {
        System.out.println("==========================================================");
        System.out.println("  TIME-TRAVEL KEY-VALUE STORE: JAVA 17 DEMONSTRATION");
        System.out.println("==========================================================");

        TimeTravelDemo demo = new TimeTravelDemo();

        System.out.println("\n1. Writing versions for key 'username':");
        demo.set("username", "Alice");
        long t1 = demo.currentTimeMicros;
        System.out.println("   Version 1 -> Alice");

        demo.set("username", "Bob");
        long t2 = demo.currentTimeMicros;
        System.out.println("   Version 2 -> Bob");

        demo.set("username", "Charlie");
        long t3 = demo.currentTimeMicros;
        System.out.println("   Version 3 -> Charlie");

        demo.delete("username");
        long t4 = demo.currentTimeMicros;
        System.out.println("   Version 4 -> Deleted (Tombstone)");

        System.out.println("\n2. Queries by Version Number:");
        System.out.println("   GET username AT VERSION 1: " + demo.getAtVersion("username", 1));
        System.out.println("   GET username AT VERSION 2: " + demo.getAtVersion("username", 2));
        System.out.println("   GET username AT VERSION 3: " + demo.getAtVersion("username", 3));
        System.out.println("   GET username AT VERSION 4: " + demo.getAtVersion("username", 4));
        System.out.println("   GET username (current):   " + demo.getCurrent("username"));

        System.out.println("\n3. Temporal Lookup (Binary Search):");
        System.out.println("   At t1 (" + t1 + " us): " + demo.getAtTime("username", t1));
        System.out.println("   At t2 (" + t2 + " us): " + demo.getAtTime("username", t2));
        System.out.println("   At t3 (" + t3 + " us): " + demo.getAtTime("username", t3));
        System.out.println("   At t4 (" + t4 + " us): " + demo.getAtTime("username", t4));

        System.out.println("\n4. Rollback to Version 2 ('Bob'):");
        demo.rollback("username", 2);
        System.out.println("   GET username (current now): " + demo.getCurrent("username"));
        System.out.println("   GET username AT VERSION 5:  " + demo.getAtVersion("username", 5));

        System.out.println("\n==========================================================");
        System.out.println("  JAVA DEMO COMPLETED SUCCESSFULLY");
        System.out.println("==========================================================");
    }
}
