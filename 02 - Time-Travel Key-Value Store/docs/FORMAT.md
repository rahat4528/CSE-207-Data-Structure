# Binary Write-Ahead Log (WAL) Format Specification

## 1. Header Layout (20 Bytes)

Every valid `.wal` file begins with a 20-byte binary header:

```text
+-------------------+-------------------+-------------------+-------------------+
|  Offset (Bytes)   |  Field Name       |  Data Type        |  Description      |
+-------------------+-------------------+-------------------+-------------------+
|  0 .. 3           |  Magic Number     |  uint32_t         |  0x564B5454 ("TTKV")|
|  4 .. 5           |  Format Version   |  uint16_t         |  Version 1        |
|  6 .. 7           |  Flags            |  uint16_t         |  Bit flags (0)    |
|  8 .. 15          |  Created Time     |  uint64_t         |  UTC microseconds |
|  16 .. 19         |  Header CRC32     |  uint32_t         |  CRC of bytes 0-15|
+-------------------+-------------------+-------------------+-------------------+
```

---

## 2. Record Framing Layout

Following the header, immutable version records are appended sequentially:

```text
+-------------------+-------------------+-------------------+-------------------+
|  Field Name       |  Size (Bytes)     |  Data Type        |  Description      |
+-------------------+-------------------+-------------------+-------------------+
|  Payload Length   |  4                |  uint32_t         |  Byte size of payload
|  Global Version ID|  8                |  uint64_t         |  Monotonic sequence ID
|  Key Version Num  |  4                |  uint32_t         |  Per-key version (1..N)
|  Key Numeric ID   |  4                |  uint32_t         |  Internal key identifier
|  Timestamp        |  8                |  uint64_t         |  UTC microseconds |
|  Prev Version ID  |  8                |  uint64_t         |  Version chain link|
|  Op Type          |  1                |  uint8_t          |  SET/UPDATE/DELETE|
|  Is Deleted       |  1                |  uint8_t          |  1=Tombstone, 0=Data
|  Expiry Timestamp |  8                |  uint64_t         |  0=No TTL, >0=Expiry
|  Key Length       |  2                |  uint16_t         |  Length of key    |
|  Key Bytes        |  Key Length       |  char[]           |  UTF-8 key string |
|  Value Length     |  4                |  uint32_t         |  Length of value  |
|  Value Bytes      |  Value Length     |  char[]           |  Raw value bytes  |
|  Author Length    |  2                |  uint16_t         |  Length of author |
|  Author Bytes     |  Author Length    |  char[]           |  Author identifier|
|  Desc Length      |  2                |  uint16_t         |  Length of desc   |
|  Desc Bytes       |  Desc Length      |  char[]           |  Change description
|  Record CRC32     |  4                |  uint32_t         |  IEEE 802.3 CRC32 |
+-------------------+-------------------+-------------------+-------------------+
```

---

## 3. Corruption & Recovery Invariants

1. **Header Validation:** If the magic number or header CRC32 does not match, loading halts immediately with `ERR_UNSUPPORTED_FORMAT` or `ERR_CHECKSUM_MISMATCH`.
2. **Mid-File Corruption:** A flipped bit or corrupted payload length occurring in the middle of the file halts recovery with an exact byte offset report. It is never silently ignored.
3. **Torn Final Record:** If a write was interrupted by a power failure or process crash mid-record at the very end of the file, recovery mode safely truncates the file back to the last valid record boundary, preserving all committed history.
