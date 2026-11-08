#include "StorageEngine.h"
#include "CRC32.h"
#include <filesystem>
#include <cstring>
#include <sstream>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#include <process.h>
#define GETPID() _getpid()
#else
#include <unistd.h>
#define GETPID() getpid()
#endif

namespace ttkv {

namespace {

template <typename T>
void writePod(std::vector<uint8_t>& buf, const T& val) {
    const auto* p = reinterpret_cast<const uint8_t*>(&val);
    buf.insert(buf.end(), p, p + sizeof(T));
}

template <typename T>
bool readPod(const uint8_t*& ptr, size_t& remaining, T& val) {
    if (remaining < sizeof(T)) return false;
    std::memcpy(&val, ptr, sizeof(T));
    ptr += sizeof(T);
    remaining -= sizeof(T);
    return true;
}

void writeString16(std::vector<uint8_t>& buf, const std::string& str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    writePod(buf, len);
    buf.insert(buf.end(), str.begin(), str.end());
}

bool readString16(const uint8_t*& ptr, size_t& remaining, std::string& str) {
    uint16_t len = 0;
    if (!readPod(ptr, remaining, len)) return false;
    if (remaining < len) return false;
    str.assign(reinterpret_cast<const char*>(ptr), len);
    ptr += len;
    remaining -= len;
    return true;
}

void writeString32(std::vector<uint8_t>& buf, const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    writePod(buf, len);
    buf.insert(buf.end(), str.begin(), str.end());
}

bool readString32(const uint8_t*& ptr, size_t& remaining, std::string& str) {
    uint32_t len = 0;
    if (!readPod(ptr, remaining, len)) return false;
    if (remaining < len) return false;
    str.assign(reinterpret_cast<const char*>(ptr), len);
    ptr += len;
    remaining -= len;
    return true;
}

} // namespace

uint32_t StorageHeader::computeCrc() const {
    CRC32 crc;
    crc.update(&magic, sizeof(magic));
    crc.update(&version, sizeof(version));
    crc.update(&flags, sizeof(flags));
    crc.update(&createdTimestamp, sizeof(createdTimestamp));
    return crc.get();
}

StorageEngine::StorageEngine(std::string walFilePath, DurabilityLevel durability)
    : m_filePath(std::move(walFilePath)), m_durability(durability) {}

StorageEngine::~StorageEngine() {
    close();
}

Result<void> StorageEngine::open() {
    if (m_isOpen) return Result<void>::ok();

    std::filesystem::path p(m_filePath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    bool fileExists = std::filesystem::exists(m_filePath) && std::filesystem::file_size(m_filePath) > 0;

    m_file.open(m_filePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!m_file.is_open()) {
        // Create new file
        m_file.clear();
        m_file.open(m_filePath, std::ios::out | std::ios::binary);
        m_file.close();
        m_file.open(m_filePath, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!m_file.is_open()) {
        return Result<void>::error(ErrorCode::ERR_IO_FAILURE, "Failed to open WAL file: " + m_filePath);
    }

    if (!fileExists) {
        // Write fresh header
        StorageHeader hdr;
        hdr.createdTimestamp = 1000ULL;
        hdr.headerCrc32 = hdr.computeCrc();

        m_file.seekp(0, std::ios::beg);
        m_file.write(reinterpret_cast<const char*>(&hdr.magic), sizeof(hdr.magic));
        m_file.write(reinterpret_cast<const char*>(&hdr.version), sizeof(hdr.version));
        m_file.write(reinterpret_cast<const char*>(&hdr.flags), sizeof(hdr.flags));
        m_file.write(reinterpret_cast<const char*>(&hdr.createdTimestamp), sizeof(hdr.createdTimestamp));
        m_file.write(reinterpret_cast<const char*>(&hdr.headerCrc32), sizeof(hdr.headerCrc32));
        m_file.flush();
    } else {
        // Seek to end for append
        m_file.seekp(0, std::ios::end);
    }

    m_isOpen = true;
    return Result<void>::ok();
}

void StorageEngine::close() {
    if (m_isOpen) {
        if (m_file.is_open()) {
            flush();
            m_file.close();
        }
        m_isOpen = false;
    }
}

Result<void> StorageEngine::appendRecord(const VersionRecord& record) {
    if (!m_isOpen) {
        auto openRes = open();
        if (openRes.isError()) return openRes;
    }

    // Serialize record fields into payload buffer
    std::vector<uint8_t> payload;
    payload.reserve(256);

    uint64_t gVer = record.globalVersionId();
    uint32_t kVer = record.keyVersionNumber();
    uint32_t kId = record.keyId();
    uint64_t ts = record.timestamp();
    uint64_t prevId = record.prevVersionId();
    uint8_t op = static_cast<uint8_t>(record.opType());
    uint8_t del = record.isDeleted() ? 1 : 0;
    uint64_t exp = record.expiryTimestamp();

    writePod(payload, gVer);
    writePod(payload, kVer);
    writePod(payload, kId);
    writePod(payload, ts);
    writePod(payload, prevId);
    writePod(payload, op);
    writePod(payload, del);
    writePod(payload, exp);

    writeString16(payload, record.key());
    writeString32(payload, record.value());
    writeString16(payload, record.author());
    writeString16(payload, record.description());

    uint32_t payloadLen = static_cast<uint32_t>(payload.size());
    uint32_t crc = CRC32::calculate(payload.data(), payload.size());

    m_file.seekp(0, std::ios::end);
    m_file.write(reinterpret_cast<const char*>(&payloadLen), sizeof(payloadLen));
    m_file.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    m_file.write(reinterpret_cast<const char*>(&crc), sizeof(crc));

    if (!m_file.good()) {
        return Result<void>::error(ErrorCode::ERR_IO_FAILURE, "Failed to write record to WAL");
    }

    return flush();
}

Result<void> StorageEngine::flush() {
    if (!m_isOpen || !m_file.is_open()) return Result<void>::ok();

    m_file.flush();
    if (m_durability == DurabilityLevel::LEVEL2_SYNC) {
        return syncToDisk();
    }
    return Result<void>::ok();
}

Result<void> StorageEngine::syncToDisk() {
    if (!m_isOpen || !m_file.is_open()) return Result<void>::ok();

    m_file.flush();
#if defined(_WIN32)
    // Synchronize to physical disk using Windows API
    std::wstring wpath(m_filePath.begin(), m_filePath.end());
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
    }
#endif
    return Result<void>::ok();
}

Result<size_t> StorageEngine::recover(KeyManager& km, EventLog& el, bool recoveryMode) {
    close();

    std::ifstream in(m_filePath, std::ios::binary);
    if (!in.is_open()) {
        return Result<size_t>::error(ErrorCode::ERR_IO_FAILURE, "Cannot open WAL for recovery: " + m_filePath);
    }

    in.seekg(0, std::ios::end);
    std::streampos totalFileSize = in.tellg();
    in.seekg(0, std::ios::beg);

    if (totalFileSize < 20) {
        return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA, "WAL file is too short to contain a valid header");
    }

    // Read and verify header
    StorageHeader hdr;
    in.read(reinterpret_cast<char*>(&hdr.magic), sizeof(hdr.magic));
    in.read(reinterpret_cast<char*>(&hdr.version), sizeof(hdr.version));
    in.read(reinterpret_cast<char*>(&hdr.flags), sizeof(hdr.flags));
    in.read(reinterpret_cast<char*>(&hdr.createdTimestamp), sizeof(hdr.createdTimestamp));
    in.read(reinterpret_cast<char*>(&hdr.headerCrc32), sizeof(hdr.headerCrc32));

    if (hdr.magic != StorageHeader::kMagic) {
        return Result<size_t>::error(ErrorCode::ERR_UNSUPPORTED_FORMAT, "Invalid WAL magic bytes");
    }
    if (hdr.version != StorageHeader::kCurrentVersion) {
        return Result<size_t>::error(ErrorCode::ERR_UNSUPPORTED_FORMAT, "Unsupported WAL version");
    }
    if (hdr.headerCrc32 != hdr.computeCrc()) {
        return Result<size_t>::error(ErrorCode::ERR_CHECKSUM_MISMATCH, "WAL header CRC32 mismatch");
    }

    km.clear();
    el.clear();

    size_t recordsReplayed = 0;
    std::streampos lastValidPos = in.tellg();

    while (in.peek() != EOF) {
        std::streampos recordStartPos = in.tellg();

        uint32_t payloadLen = 0;
        in.read(reinterpret_cast<char*>(&payloadLen), sizeof(payloadLen));
        if (in.gcount() != sizeof(payloadLen)) {
            // Torn length header at EOF
            if (recoveryMode) {
                in.close();
                std::filesystem::resize_file(m_filePath, static_cast<uintmax_t>(lastValidPos));
                return open().isOk() ? Result<size_t>::ok(recordsReplayed)
                                     : Result<size_t>::error(ErrorCode::ERR_IO_FAILURE);
            }
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                         "Torn record length detected at offset " + std::to_string(recordStartPos));
        }

        // Check if this record extends beyond EOF (torn tail)
        std::streampos requiredRecordEnd = recordStartPos +
            static_cast<std::streampos>(sizeof(payloadLen)) +
            static_cast<std::streampos>(payloadLen) +
            static_cast<std::streampos>(sizeof(uint32_t));

        if (requiredRecordEnd > totalFileSize) {
            if (recoveryMode) {
                in.close();
                std::filesystem::resize_file(m_filePath, static_cast<uintmax_t>(lastValidPos));
                return open().isOk() ? Result<size_t>::ok(recordsReplayed)
                                     : Result<size_t>::error(ErrorCode::ERR_IO_FAILURE);
            }
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                         "Torn record at EOF at offset " + std::to_string(recordStartPos));
        }

        if (payloadLen > 64 * 1024 * 1024) { // 64 MB sanity cap
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                         "Record length exceeds safety limits at offset " + std::to_string(recordStartPos));
        }

        std::vector<uint8_t> payload(payloadLen);
        in.read(reinterpret_cast<char*>(payload.data()), payloadLen);
        if (static_cast<uint32_t>(in.gcount()) != payloadLen) {
            // Truncated payload
            if (recoveryMode && (in.peek() == EOF)) {
                in.close();
                std::filesystem::resize_file(m_filePath, static_cast<uintmax_t>(lastValidPos));
                return open().isOk() ? Result<size_t>::ok(recordsReplayed)
                                     : Result<size_t>::error(ErrorCode::ERR_IO_FAILURE);
            }
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                         "Torn payload truncated at offset " + std::to_string(recordStartPos));
        }

        uint32_t storedCrc = 0;
        in.read(reinterpret_cast<char*>(&storedCrc), sizeof(storedCrc));
        if (in.gcount() != sizeof(storedCrc)) {
            if (recoveryMode) {
                in.close();
                std::filesystem::resize_file(m_filePath, static_cast<uintmax_t>(lastValidPos));
                return open().isOk() ? Result<size_t>::ok(recordsReplayed)
                                     : Result<size_t>::error(ErrorCode::ERR_IO_FAILURE);
            }
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                         "Torn CRC32 trailer at offset " + std::to_string(recordStartPos));
        }

        uint32_t computedCrc = CRC32::calculate(payload.data(), payload.size());
        if (storedCrc != computedCrc) {
            // Check whether this is at the end of file (torn tail) or in the middle
            bool atEof = (in.peek() == EOF);
            if (atEof && recoveryMode) {
                in.close();
                std::filesystem::resize_file(m_filePath, static_cast<uintmax_t>(lastValidPos));
                return open().isOk() ? Result<size_t>::ok(recordsReplayed)
                                     : Result<size_t>::error(ErrorCode::ERR_IO_FAILURE);
            }
            std::string typeStr = atEof ? "Torn final record" : "Mid-file record corruption";
            return Result<size_t>::error(ErrorCode::ERR_CHECKSUM_MISMATCH,
                                         typeStr + " with CRC mismatch at offset " + std::to_string(recordStartPos));
        }

        // Deserialize payload
        const uint8_t* ptr = payload.data();
        size_t rem = payload.size();

        uint64_t gVer = 0;
        uint32_t kVer = 0, kId = 0;
        uint64_t ts = 0, prevId = 0;
        uint8_t opVal = 0, delVal = 0;
        uint64_t exp = 0;
        std::string key, val, auth, desc;

        if (!readPod(ptr, rem, gVer) ||
            !readPod(ptr, rem, kVer) ||
            !readPod(ptr, rem, kId) ||
            !readPod(ptr, rem, ts) ||
            !readPod(ptr, rem, prevId) ||
            !readPod(ptr, rem, opVal) ||
            !readPod(ptr, rem, delVal) ||
            !readPod(ptr, rem, exp) ||
            !readString16(ptr, rem, key) ||
            !readString32(ptr, rem, val) ||
            !readString16(ptr, rem, auth) ||
            !readString16(ptr, rem, desc)) {
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA,
                                         "Malformed record structure at offset " + std::to_string(recordStartPos));
        }

        VersionRecord rec(gVer, kVer, kId, key, val, static_cast<OpType>(opVal),
                          ts, prevId, (delVal != 0), exp, auth, desc);

        auto applyRes = km.applyRecord(rec);
        if (applyRes.isError()) {
            return Result<size_t>::error(applyRes.error(),
                                         "Recovery failed applying record at offset " +
                                         std::to_string(recordStartPos) + ": " + applyRes.message());
        }

        el.appendEvent(rec);
        recordsReplayed++;
        lastValidPos = in.tellg();
    }

    in.close();
    return open().isOk() ? Result<size_t>::ok(recordsReplayed)
                         : Result<size_t>::error(ErrorCode::ERR_IO_FAILURE);
}

Result<void> StorageEngine::atomicWriteFile(const std::string& targetPath, const std::string& content) {
    int pid = GETPID();
    std::string tmpPath = targetPath + ".tmp." + std::to_string(pid);

    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return Result<void>::error(ErrorCode::ERR_IO_FAILURE, "Failed to create temp file: " + tmpPath);
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.flush();
        if (!out.good()) {
            out.close();
            std::filesystem::remove(tmpPath);
            return Result<void>::error(ErrorCode::ERR_IO_FAILURE, "Failed writing content to temp file");
        }
        out.close();
    }

    // Atomic replacement
    std::error_code ec;
    std::filesystem::rename(tmpPath, targetPath, ec);
    if (ec) {
        // Fallback for Windows if target exists
        std::filesystem::remove(targetPath, ec);
        std::filesystem::rename(tmpPath, targetPath, ec);
        if (ec) {
            std::filesystem::remove(tmpPath);
            return Result<void>::error(ErrorCode::ERR_IO_FAILURE, "Atomic rename failed: " + ec.message());
        }
    }

    return Result<void>::ok();
}

} // namespace ttkv
