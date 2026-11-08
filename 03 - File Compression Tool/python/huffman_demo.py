#!/usr/bin/env python3
"""
Project 03: File Compression Tool — Huffman Coding Engine
Python 3 Companion Reference Demonstration
Demonstrates greedy Huffman tree construction, prefix-free binary code generation,
and exact round-trip compression and decompression.
"""

import heapq
from collections import Counter

class HuffmanNode:
    def __init__(self, char, freq, left=None, right=None):
        self.char = char
        self.freq = freq
        self.left = left
        self.right = right

    def __lt__(self, other):
        # Tie-breaking by frequency, then character if present
        if self.freq != other.freq:
            return self.freq < other.freq
        s_char = self.char if self.char is not None else ""
        o_char = other.char if other.char is not None else ""
        return s_char < o_char

def build_huffman_tree(text: str):
    if not text:
        return None, {}
    
    freqs = Counter(text)
    heap = [HuffmanNode(char, freq) for char, freq in freqs.items()]
    heapq.heapify(heap)

    if len(heap) == 1:
        # Single symbol special case: 1-bit code '0'
        node = heapq.heappop(heap)
        return node, {node.char: "0"}

    while len(heap) > 1:
        left = heapq.heappop(heap)
        right = heapq.heappop(heap)
        merged = HuffmanNode(None, left.freq + right.freq, left, right)
        heapq.heappush(heap, merged)

    root = heap[0]
    codes = {}

    def generate_codes(node, current_code):
        if not node:
            return
        if node.char is not None:
            codes[node.char] = current_code or "0"
            return
        generate_codes(node.left, current_code + "0")
        generate_codes(node.right, current_code + "1")

    generate_codes(root, "")
    return root, codes

def compress_string(text: str, codes: dict) -> str:
    return "".join(codes[ch] for ch in text)

def decompress_bitstring(bitstring: str, root: HuffmanNode, original_length: int) -> str:
    if not root or original_length == 0:
        return ""
    if root.char is not None:
        # Single character repeated
        return root.char * original_length

    result = []
    curr = root
    for bit in bitstring:
        curr = curr.right if bit == "1" else curr.left
        if curr.char is not None:
            result.append(curr.char)
            curr = root
            if len(result) == original_length:
                break
    return "".join(result)

def main():
    sample_text = "Huffman coding is an optimal prefix code used for lossless data compression!"
    print("=" * 70)
    print("  PROJECT 03: HUFFMAN CODING DEMO (PYTHON 3)")
    print("=" * 70)
    print(f"Original Text   : {sample_text}")
    print(f"Original Size   : {len(sample_text)} bytes ({len(sample_text) * 8} bits)")

    root, codes = build_huffman_tree(sample_text)
    bitstring = compress_string(sample_text, codes)
    print(f"Compressed Bits : {len(bitstring)} bits ({len(bitstring) / 8:.2f} bytes)")
    ratio = len(bitstring) / (len(sample_text) * 8)
    print(f"Bit Compression : {ratio:.4f} ({(1 - ratio) * 100:.2f}% space savings)")

    restored = decompress_bitstring(bitstring, root, len(sample_text))
    print(f"Restored Text   : {restored}")
    assert restored == sample_text, "Round-trip restoration check failed!"
    print("\n[VERIFICATION] Python 3 Round-trip matches 100% byte-for-byte!")
    print("=" * 70)

if __name__ == "__main__":
    main()
