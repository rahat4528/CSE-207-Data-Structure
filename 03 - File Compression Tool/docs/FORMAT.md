# Binary Archive Specification — .huff Format (Version 1)

## 1. File Layout Overview

All integer values are stored in strictly **little-endian** byte order.

```text
+-----------------------------------------------------------------------------------+
|                            HEADER SECTION (Variable Length)                       |
+-------------------+-----------------+--------------------+------------------------+
| Field Name        | Type / Size     | Value / Range      | Description            |
+-------------------+-----------------+--------------------+------------------------+
| Magic Signature   | uint8_t[4]      | "HUFF" (0x48554646)| File format identifier |
| Format Version    | uint8_t         | 0x01               | Version 1 specification|
| Compression Mode  | uint8_t         | 0x00 / 0x01        | 0x00=Huffman, 0x01=Raw |
| Reserved Flags    | uint16_t        | 0x0000             | Must be zero           |
| Original Size     | uint64_t        | 0 to 2^64-1        | Uncompressed file size |
| Modification Time | uint64_t        | UTC epoch seconds  | File timestamp         |
| Filename Length   | uint16_t        | 0 to 1024 bytes    | Length of filename     |
| Original Filename | char[NL]        | UTF-8 string       | Sanitized base name    |
| Alphabet Count (A)| uint16_t        | 0 to 256           | Distinct symbol count  |
| Symbol Frequencies| (uint8_t,       | Active pairs:      | Symbol byte +          |
|                   |  uint64_t)[A]   | sym + frequency    | 64-bit frequency       |
| Padding Bits      | uint8_t         | 0 to 7             | Unused bits in payload |
| Header CRC32      | uint32_t        | IEEE 802.3 CRC32   | Checksum over header   |
+-------------------+-----------------+--------------------+------------------------+
|                            PAYLOAD SECTION (Variable Length)                      |
+-------------------+-----------------+--------------------+------------------------+
| Encoded Bitstream | uint8_t[M]      | Packed bitstream   | MSB-first packed bytes |
+-------------------+-----------------+--------------------+------------------------+
|                            TRAILER SECTION (4 Bytes)                              |
+-------------------+-----------------+--------------------+------------------------+
| Data CRC32        | uint32_t        | IEEE 802.3 CRC32   | CRC32 of uncompressed  |
+-------------------+-----------------+--------------------+------------------------+
```

---

## 2. Detailed Field Specifications

### 2.1 Magic Signature (4 Bytes)
- **Offset:** `0x00`
- **Value:** `{'H', 'U', 'F', 'F'}` (`0x48, 0x55, 0x46, 0x46`).
- **Validation:** Readers must immediately abort with `ERR_INVALID_SIGNATURE` if any byte mismatches.

### 2.2 Format Version (1 Byte)
- **Offset:** `0x04`
- **Value:** `0x01`.
- **Validation:** Unknown or higher versions are rejected with `ERR_UNSUPPORTED_VERSION`.

### 2.3 Compression Mode (1 Byte)
- **Offset:** `0x05`
- **Values:**
  - `0x00`: Standard Huffman Coding.
  - `0x01`: Raw Stored Fallback (used when compression overhead exceeds uncompressed size).

### 2.4 Reserved Flags (2 Bytes, Little-Endian)
- **Offset:** `0x06`
- **Value:** `0x0000`.
- **Validation:** Must be zero. Non-zero values are rejected with `ERR_CORRUPTED_HEADER`.

### 2.5 Original File Size (8 Bytes, Little-Endian)
- **Offset:** `0x08`
- **Value:** `uint64_t` byte count of original file before compression.

### 2.6 Modification Time (8 Bytes, Little-Endian)
- **Offset:** `0x10`
- **Value:** `uint64_t` seconds since Unix epoch (UTC).

### 2.7 Filename Length & Name
- **Length:** `uint16_t` (2 bytes).
- **Data:** `char[NL]` sanitized UTF-8 string containing base name only (no paths, drives, or `../`).

### 2.8 Alphabet Count & Frequency Records
- **Alphabet Count:** `uint16_t` (2 bytes, range $0 \dots 256$).
- **Records:** Exactly `alphabetSize` records of:
  - `symbol`: `uint8_t` (1 byte)
  - `frequency`: `uint64_t` (8 bytes, little-endian)
- **Validation Invariant:** $\sum_{i=1}^{A} \text{freq}_i = \text{originalSize}$.

### 2.9 Padding Bits (1 Byte)
- **Value:** Number of zero-padding bits added to the final payload byte (range $0 \dots 7$).

### 2.10 Header CRC32 (4 Bytes, Little-Endian)
- Computed over all preceding header bytes from offset `0x00` to the padding byte.
- Protects metadata against corruption before any payload allocations.

---

## 3. Payload Section & Bit Order
- Bits are packed into bytes from **Most Significant Bit (MSB, bit 7)** down to **Least Significant Bit (LSB, bit 0)**.
- For incomplete final bytes, unused bits are padded with strictly `0` bits.
- Exact payload bit count:
  $$\text{Payload Bits} = \left(\sum_{i=0}^{255} \text{freq}_i \times \text{length}_i\right)$$
- Total payload byte count:
  $$\text{Payload Bytes} = \frac{\text{Payload Bits} + \text{Padding Bits}}{8}$$

---

## 4. Trailer Section (4 Bytes)
- **Data CRC32:** IEEE 802.3 CRC32 calculated over the original uncompressed byte content.
- Decompressor compares this value against the running CRC32 of restored bytes. Any mismatch results in atomic file deletion and `ERR_DATA_CRC_MISMATCH`.
