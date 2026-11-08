#pragma once

#include "Common.h"
#include "DataModel.h"
#include "KeyManager.h"
#include "EventLog.h"
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

namespace ttkv {

enum class DurabilityLevel {
    LEVEL1_FLUSH = 1,   // C++ stream flush (buffered by OS page cache, process-crash safe)
    LEVEL2_SYNC = 2     // OS physical disk flush (_commit/fsync, power-loss safe)
};

struct StorageHeader {
    static constexpr uint32_t kMagic = 0x564B5454; // "TTKV" in little-endian
    static constexpr uint16_t kCurrentVersion = 1;

    uint32_t magic{kMagic};
    uint16_t version{kCurrentVersion};
    uint16_t flags{0};
    uint64_t createdTimestamp{0};
    uint32_t headerCrc32{0};

    uint32_t computeCrc() const;
};

class StorageEngine {
public:
    explicit StorageEngine(std::string walFilePath,
                           DurabilityLevel durability = DurabilityLevel::LEVEL1_FLUSH);
    ~StorageEngine();

    // Opens or creates the WAL file
    Result<void> open();

    // Closes the WAL file
    void close();

    // Appends a committed version record to the WAL on disk
    Result<void> appendRecord(const VersionRecord& record);

    // Flushes pending writes according to active durability level
    Result<void> flush();

    // Replays WAL file into in-memory KeyManager and EventLog on startup/restart
    // If recoveryMode is true, a torn final tail will be truncated up to the last valid record
    Result<size_t> recover(KeyManager& km, EventLog& el, bool recoveryMode = false);

    // Forces a physical disk sync regardless of default durability level
    Result<void> syncToDisk();

    const std::string& filePath() const { return m_filePath; }
    DurabilityLevel durability() const { return m_durability; }
    void setDurability(DurabilityLevel d) { m_durability = d; }

    // Utilities for atomic file writing via temporary files
    static Result<void> atomicWriteFile(const std::string& targetPath, const std::string& content);

private:
    std::string m_filePath;
    DurabilityLevel m_durability;
    std::fstream m_file;
    bool m_isOpen{false};
};

} // namespace ttkv
