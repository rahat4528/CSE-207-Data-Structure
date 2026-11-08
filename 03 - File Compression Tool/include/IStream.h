#ifndef HUFFMAN_ISTREAM_H
#define HUFFMAN_ISTREAM_H

#include "Common.h"
#include <fstream>
#include <vector>
#include <cstring>

namespace HuffmanEngine {

class IInputStream {
public:
    virtual ~IInputStream() = default;
    virtual size_t read(uint8_t* buffer, size_t size) = 0;
    virtual bool isEof() const = 0;
    virtual filesize_t totalSize() const = 0;
    virtual void rewind() = 0;
};

class IOutputStream {
public:
    virtual ~IOutputStream() = default;
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;
    virtual void flush() = 0;
    virtual filesize_t bytesWritten() const = 0;
};

// In-Memory Stream for hermetic testing
class MemoryInputStream : public IInputStream {
private:
    std::vector<uint8_t> data;
    size_t cursor{0};

public:
    explicit MemoryInputStream(std::vector<uint8_t> bytes) : data(std::move(bytes)), cursor(0) {}
    explicit MemoryInputStream(const std::string& str) 
        : data(str.begin(), str.end()), cursor(0) {}

    size_t read(uint8_t* buffer, size_t size) override {
        if (cursor >= data.size()) return 0;
        size_t available = data.size() - cursor;
        size_t toRead = (size < available) ? size : available;
        std::memcpy(buffer, data.data() + cursor, toRead);
        cursor += toRead;
        return toRead;
    }

    bool isEof() const override {
        return cursor >= data.size();
    }

    filesize_t totalSize() const override {
        return static_cast<filesize_t>(data.size());
    }

    void rewind() override {
        cursor = 0;
    }
};

class MemoryOutputStream : public IOutputStream {
private:
    std::vector<uint8_t> buffer;

public:
    MemoryOutputStream() = default;

    size_t write(const uint8_t* data, size_t size) override {
        buffer.insert(buffer.end(), data, data + size);
        return size;
    }

    void flush() override {}

    filesize_t bytesWritten() const override {
        return static_cast<filesize_t>(buffer.size());
    }

    const std::vector<uint8_t>& getBuffer() const { return buffer; }
    std::vector<uint8_t> takeBuffer() { return std::move(buffer); }
    void clear() { buffer.clear(); }
};

// Stream wrapper that limits reads to an exact byte budget
class LimitedInputStream : public IInputStream {
private:
    IInputStream& parent;
    filesize_t bytesRemaining{0};

public:
    LimitedInputStream(IInputStream& p, filesize_t limit)
        : parent(p), bytesRemaining(limit) {}

    size_t read(uint8_t* buffer, size_t size) override {
        if (bytesRemaining == 0) return 0;
        size_t toRead = (size < bytesRemaining) ? size : static_cast<size_t>(bytesRemaining);
        size_t actual = parent.read(buffer, toRead);
        bytesRemaining -= actual;
        return actual;
    }

    bool isEof() const override {
        return (bytesRemaining == 0) || parent.isEof();
    }

    filesize_t totalSize() const override {
        return bytesRemaining;
    }

    void rewind() override {
        // Substream rewind not required
    }
};

} // namespace HuffmanEngine

#endif // HUFFMAN_ISTREAM_H
