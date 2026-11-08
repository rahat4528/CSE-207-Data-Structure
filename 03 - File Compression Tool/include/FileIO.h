#ifndef HUFFMAN_FILE_IO_H
#define HUFFMAN_FILE_IO_H

#include "IStream.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace HuffmanEngine {

class FileInputStream : public IInputStream {
private:
    std::filesystem::path filePath;
    mutable std::ifstream stream;
    filesize_t fileSize{0};

public:
    explicit FileInputStream(std::filesystem::path path);
    ~FileInputStream() override;

    bool isOpen() const { return stream.is_open(); }
    size_t read(uint8_t* buffer, size_t size) override;
    bool isEof() const override;
    filesize_t totalSize() const override { return fileSize; }
    void rewind() override;
    void close();
};

class FileOutputStream : public IOutputStream {
private:
    std::filesystem::path finalPath;
    std::filesystem::path tempPath;
    std::ofstream stream;
    filesize_t writtenBytes{0};
    bool committed{false};

public:
    FileOutputStream(std::filesystem::path targetPath, bool atomicTemp = true);
    ~FileOutputStream() override;

    bool isOpen() const { return stream.is_open(); }
    size_t write(const uint8_t* data, size_t size) override;
    void flush() override;
    filesize_t bytesWritten() const override { return writtenBytes; }
    
    // Commits temporary file to final path atomically
    Result<void> commit();
    
    // Aborts and deletes temporary file
    void abort();
};

} // namespace HuffmanEngine

#endif // HUFFMAN_FILE_IO_H
