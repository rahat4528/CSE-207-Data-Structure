import java.util.*;

/**
 * Project 03: File Compression Tool — Huffman Coding Engine
 * Java 17 Companion Reference Demonstration
 */
public class HuffmanDemo {

    static class HuffmanNode implements Comparable<HuffmanNode> {
        final Character ch;
        final long freq;
        final HuffmanNode left;
        final HuffmanNode right;

        HuffmanNode(Character ch, long freq, HuffmanNode left, HuffmanNode right) {
            this.ch = ch;
            this.freq = freq;
            this.left = left;
            this.right = right;
        }

        boolean isLeaf() {
            return left == null && right == null;
        }

        @Override
        public int compareTo(HuffmanNode other) {
            if (this.freq != other.freq) {
                return Long.compare(this.freq, other.freq);
            }
            char c1 = (this.ch != null) ? this.ch : '\0';
            char c2 = (other.ch != null) ? other.ch : '\0';
            return Character.compare(c1, c2);
        }
    }

    public static void generateCodes(HuffmanNode node, String prefix, Map<Character, String> codeMap) {
        if (node == null) return;
        if (node.isLeaf()) {
            codeMap.put(node.ch, prefix.isEmpty() ? "0" : prefix);
            return;
        }
        generateCodes(node.left, prefix + "0", codeMap);
        generateCodes(node.right, prefix + "1", codeMap);
    }

    public static void main(String[] args) {
        String text = "Huffman coding is an optimal prefix code used for lossless data compression!";
        System.out.println("======================================================================");
        System.out.println("  PROJECT 03: HUFFMAN CODING DEMO (JAVA 17)");
        System.out.println("======================================================================");
        System.out.println("Original Text   : " + text);
        System.out.println("Original Size   : " + text.length() + " bytes (" + (text.length() * 8) + " bits)");

        // 1. Calculate frequencies
        Map<Character, Long> freqs = new HashMap<>();
        for (char c : text.toCharArray()) {
            freqs.put(c, freqs.getOrDefault(c, 0L) + 1);
        }

        // 2. Build Min-Heap
        PriorityQueue<HuffmanNode> pq = new PriorityQueue<>();
        for (Map.Entry<Character, Long> entry : freqs.entrySet()) {
            pq.add(new HuffmanNode(entry.getKey(), entry.getValue(), null, null));
        }

        // 3. Build Huffman Tree
        while (pq.size() > 1) {
            HuffmanNode left = pq.poll();
            HuffmanNode right = pq.poll();
            pq.add(new HuffmanNode(null, left.freq + right.freq, left, right));
        }
        HuffmanNode root = pq.peek();

        // 4. Generate Codes
        Map<Character, String> codeMap = new HashMap<>();
        generateCodes(root, "", codeMap);

        // 5. Encode
        StringBuilder encoded = new StringBuilder();
        for (char c : text.toCharArray()) {
            encoded.append(codeMap.get(c));
        }

        System.out.println("Compressed Bits : " + encoded.length() + " bits (" + (encoded.length() / 8.0) + " bytes)");
        double ratio = (double) encoded.length() / (text.length() * 8.0);
        System.out.printf("Bit Compression : %.4f (%.2f%% space savings)%n", ratio, (1.0 - ratio) * 100.0);

        // 6. Decode
        StringBuilder decoded = new StringBuilder();
        HuffmanNode curr = root;
        for (int i = 0; i < encoded.length(); ++i) {
            char bit = encoded.charAt(i);
            curr = (bit == '1') ? curr.right : curr.left;
            if (curr.isLeaf()) {
                decoded.append(curr.ch);
                curr = root;
                if (decoded.length() == text.length()) break;
            }
        }

        System.out.println("Restored Text   : " + decoded);
        if (text.equals(decoded.toString())) {
            System.out.println("\n[VERIFICATION] Java 17 Round-trip matches 100% byte-for-byte!");
        } else {
            System.err.println("[ERROR] Decode mismatch!");
        }
        System.out.println("======================================================================");
    }
}
