#include "FileIO.h"
#include <iostream>
#include <random>

namespace HuffmanEngine {

FileInputStream::FileInputStream(std::filesystem::path path) : filePath(std::move(path)) {
    if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
        fileSize = static_cast<filesize_t>(std::filesystem::file_size(filePath));
        stream.open(filePath, std::ios::binary);
    }
}

FileInputStream::~FileInputStream() {
    close();
}

size_t FileInputStream::read(uint8_t* buffer, size_t size) {
    if (!stream.is_open()) return 0;
    stream.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(size));
    return static_cast<size_t>(stream.gcount());
}

bool FileInputStream::isEof() const {
    if (!stream.is_open()) return true;
    return stream.eof() || (stream.peek() == EOF);
}

void FileInputStream::rewind() {
    if (stream.is_open()) {
        stream.clear();
        stream.seekg(0, std::ios::beg);
    }
}

void FileInputStream::close() {
    if (stream.is_open()) {
        stream.close();
    }
}

// FileOutputStream with atomic temporary commit
FileOutputStream::FileOutputStream(std::filesystem::path targetPath, bool atomicTemp)
    : finalPath(std::move(targetPath)), committed(!atomicTemp) {
    if (atomicTemp) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(100000, 999999);
        tempPath = finalPath;
        tempPath += ".tmp." + std::to_string(dist(gen));
        stream.open(tempPath, std::ios::binary | std::ios::trunc);
    } else {
        stream.open(finalPath, std::ios::binary | std::ios::trunc);
    }
}

FileOutputStream::~FileOutputStream() {
    if (stream.is_open()) {
        stream.close();
    }
    if (!committed && !tempPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
    }
}

size_t FileOutputStream::write(const uint8_t* data, size_t size) {
    if (!stream.is_open() || size == 0) return 0;
    stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (stream.good()) {
        writtenBytes += size;
        return size;
    }
    return 0;
}

void FileOutputStream::flush() {
    if (stream.is_open()) {
        stream.flush();
    }
}

Result<void> FileOutputStream::commit() {
    if (committed) return Result<void>::ok();
    flush();
    if (stream.is_open()) {
        stream.close();
    }
    std::error_code ec;
    std::filesystem::rename(tempPath, finalPath, ec);
    if (ec) {
        // Fallback to copy and remove on cross-device link or rename lock
        std::filesystem::copy_file(tempPath, finalPath, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tempPath);
        if (ec) {
            return Result<void>::fail(ErrorCode::ERR_FILE_WRITE_FAILED, "Failed to commit atomic output file: " + ec.message());
        }
    }
    committed = true;
    return Result<void>::ok();
}

void FileOutputStream::abort() {
    if (stream.is_open()) {
        stream.close();
    }
    if (!committed && !tempPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
    }
}

} // namespace HuffmanEngine
