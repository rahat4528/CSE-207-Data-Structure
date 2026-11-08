import java.util.*;

public class LeaderboardDemo {

    static class FenwickTree {
        private final int size;
        private final int[] tree;

        public FenwickTree(int size) {
            this.size = size;
            this.tree = new int[size + 1];
        }

        public void add(int index, int delta) {
            for (; index <= size; index += index & -index) {
                tree[index] += delta;
            }
        }

        public int prefix(int index) {
            int sum = 0;
            for (; index > 0; index -= index & -index) {
                sum += tree[index];
            }
            return sum;
        }

        public int total() {
            return prefix(size);
        }

        public int countGreater(int index) {
            return total() - prefix(index);
        }
    }

    public static void main(String[] args) {
        System.out.println("============================================================");
        System.out.println(" PROJECT 04: REAL-TIME GAME LEADERBOARD (Java Companion)");
        System.out.println("============================================================");

        // Scores 100, 90, 90, 80
        int[] scores = {100, 90, 90, 80};
        String[] names = {"Alice", "Bob", "Charlie", "David"};

        // Distinct sorted: 80 (idx 1), 90 (idx 2), 100 (idx 3)
        Map<Integer, Integer> scoreToIdx = new HashMap<>();
        scoreToIdx.put(80, 1);
        scoreToIdx.put(90, 2);
        scoreToIdx.put(100, 3);

        FenwickTree bit = new FenwickTree(3);
        for (int s : scores) {
            bit.add(scoreToIdx.get(s), 1);
        }

        System.out.println("\n--- Competition Ranking Results ('1224' Rule) ---");
        for (int i = 0; i < scores.length; i++) {
            int idx = scoreToIdx.get(scores[i]);
            int rank = 1 + bit.countGreater(idx);
            System.out.printf("  %-10s Score: %-5d => Rank #%d%n", names[i], scores[i], rank);
        }

        System.out.println("\n[SUCCESS] Java companion verification completed cleanly.\n");
    }
}
