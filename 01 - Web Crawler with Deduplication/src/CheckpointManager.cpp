#include "CheckpointManager.h"
#include "CRC32.h"
#include <fstream>
#include <filesystem>

namespace crawler {

namespace {

void writeString(std::ostream& os, const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) {
        os.write(str.data(), len);
    }
}

std::string readString(std::istream& is) {
    uint32_t len = 0;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0) return "";
    std::string s(len, '\0');
    is.read(&s[0], len);
    return s;
}

} // namespace

Result<void> CheckpointManager::save(const std::string& filePath, const CrawlerEngine& engine) {
    std::string tmpPath = filePath + ".tmp";
    std::filesystem::path p(tmpPath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ostringstream payload(std::ios::binary);

    // 1. Visited URLs
    auto visitedList = engine.visitedSet().getAll();
    uint32_t visitedCount = static_cast<uint32_t>(visitedList.size());
    payload.write(reinterpret_cast<const char*>(&visitedCount), sizeof(visitedCount));
    for (const auto& url : visitedList) {
        writeString(payload, url);
    }

    // 2. Bloom Filter raw bits
    const auto& bloom = engine.bloomFilter();
    size_t numBits = bloom.bitCount();
    size_t numHashes = bloom.hashCount();
    size_t itemCount = bloom.itemCount();
    const auto& rawBits = bloom.rawBits();
    uint64_t wordsCount = rawBits.size();

    payload.write(reinterpret_cast<const char*>(&numBits), sizeof(numBits));
    payload.write(reinterpret_cast<const char*>(&numHashes), sizeof(numHashes));
    payload.write(reinterpret_cast<const char*>(&itemCount), sizeof(itemCount));
    payload.write(reinterpret_cast<const char*>(&wordsCount), sizeof(wordsCount));
    if (wordsCount > 0) {
        payload.write(reinterpret_cast<const char*>(rawBits.data()), static_cast<std::streamsize>(wordsCount * sizeof(uint64_t)));
    }

    // 3. Crawled Pages
    auto pages = engine.storage().getAllPages();
    uint32_t pageCount = static_cast<uint32_t>(pages.size());
    payload.write(reinterpret_cast<const char*>(&pageCount), sizeof(pageCount));
    for (const auto& page : pages) {
        writeString(payload, page.url);
        payload.write(reinterpret_cast<const char*>(&page.statusCode), sizeof(page.statusCode));
        payload.write(reinterpret_cast<const char*>(&page.depth), sizeof(page.depth));
        writeString(payload, page.title);
        writeString(payload, page.metaDescription);
        payload.write(reinterpret_cast<const char*>(&page.contentLength), sizeof(page.contentLength));
        payload.write(reinterpret_cast<const char*>(&page.contentHash), sizeof(page.contentHash));
        payload.write(reinterpret_cast<const char*>(&page.simHash), sizeof(page.simHash));

        uint32_t outLinkCount = static_cast<uint32_t>(page.outLinks.size());
        payload.write(reinterpret_cast<const char*>(&outLinkCount), sizeof(outLinkCount));
        for (const auto& link : page.outLinks) {
            writeString(payload, link);
        }
    }

    std::string payloadBytes = payload.str();
    uint32_t checksum = getCRC32Instance().calculate(payloadBytes);

    // Write file with atomic rename
    std::ofstream ofs(tmpPath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        return Result<void>::error(StatusCode::IO_ERROR, "Failed to create checkpoint file: " + tmpPath);
    }

    // Header: Magic (4) + Version (2)
    uint32_t magic = MAGIC;
    uint16_t version = VERSION;
    ofs.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Payload
    ofs.write(payloadBytes.data(), static_cast<std::streamsize>(payloadBytes.size()));

    // Trailing CRC32
    ofs.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
    ofs.flush();
    ofs.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec) {
        return Result<void>::error(StatusCode::IO_ERROR, "Atomic rename failed: " + ec.message());
    }

    return Result<void>::ok();
}

Result<void> CheckpointManager::load(const std::string& filePath, CrawlerEngine& engine) {
    if (!std::filesystem::exists(filePath)) {
        return Result<void>::error(StatusCode::IO_ERROR, "Checkpoint file does not exist: " + filePath);
    }

    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        return Result<void>::error(StatusCode::IO_ERROR, "Cannot open checkpoint file: " + filePath);
    }

    uint32_t magic = 0;
    uint16_t version = 0;
    ifs.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != MAGIC || version != VERSION) {
        return Result<void>::error(StatusCode::CHECKPOINT_CORRUPTED, "Invalid checkpoint magic or version");
    }

    size_t fileSize = std::filesystem::file_size(filePath);
    if (fileSize < sizeof(magic) + sizeof(version) + sizeof(uint32_t)) {
        return Result<void>::error(StatusCode::CHECKPOINT_CORRUPTED, "Truncated checkpoint file");
    }

    size_t payloadSize = fileSize - (sizeof(magic) + sizeof(version) + sizeof(uint32_t));
    std::string payloadBytes(payloadSize, '\0');
    ifs.read(&payloadBytes[0], static_cast<std::streamsize>(payloadSize));

    uint32_t expectedCrc = 0;
    ifs.read(reinterpret_cast<char*>(&expectedCrc), sizeof(expectedCrc));

    uint32_t actualCrc = getCRC32Instance().calculate(payloadBytes);
    if (actualCrc != expectedCrc) {
        return Result<void>::error(StatusCode::CHECKPOINT_CORRUPTED, "CRC32 checksum mismatch in checkpoint");
    }

    std::istringstream stream(payloadBytes, std::ios::binary);

    // 1. Visited URLs
    uint32_t visitedCount = 0;
    stream.read(reinterpret_cast<char*>(&visitedCount), sizeof(visitedCount));
    for (uint32_t i = 0; i < visitedCount; ++i) {
        std::string u = readString(stream);
        const_cast<ExactVisitedSet&>(engine.visitedSet()).insert(u);
    }

    // 2. Bloom Filter raw bits
    size_t numBits = 0, numHashes = 0, itemCount = 0;
    uint64_t wordsCount = 0;
    stream.read(reinterpret_cast<char*>(&numBits), sizeof(numBits));
    stream.read(reinterpret_cast<char*>(&numHashes), sizeof(numHashes));
    stream.read(reinterpret_cast<char*>(&itemCount), sizeof(itemCount));
    stream.read(reinterpret_cast<char*>(&wordsCount), sizeof(wordsCount));

    std::vector<uint64_t> rawBits(wordsCount);
    if (wordsCount > 0) {
        stream.read(reinterpret_cast<char*>(rawBits.data()), static_cast<std::streamsize>(wordsCount * sizeof(uint64_t)));
        const_cast<BloomFilter&>(engine.bloomFilter()).loadRawBits(std::move(rawBits), numBits, numHashes, itemCount);
    }


    // 3. Crawled Pages
    uint32_t pageCount = 0;
    stream.read(reinterpret_cast<char*>(&pageCount), sizeof(pageCount));
    for (uint32_t i = 0; i < pageCount; ++i) {
        CrawledPage page;
        page.url = readString(stream);
        stream.read(reinterpret_cast<char*>(&page.statusCode), sizeof(page.statusCode));
        stream.read(reinterpret_cast<char*>(&page.depth), sizeof(page.depth));
        page.title = readString(stream);
        page.metaDescription = readString(stream);
        stream.read(reinterpret_cast<char*>(&page.contentLength), sizeof(page.contentLength));
        stream.read(reinterpret_cast<char*>(&page.contentHash), sizeof(page.contentHash));
        stream.read(reinterpret_cast<char*>(&page.simHash), sizeof(page.simHash));

        uint32_t outLinkCount = 0;
        stream.read(reinterpret_cast<char*>(&outLinkCount), sizeof(outLinkCount));
        for (uint32_t j = 0; j < outLinkCount; ++j) {
            std::string link = readString(stream);
            page.outLinks.push_back(link);
            const_cast<CrawlGraph&>(engine.graph()).addEdge(page.url, link);
        }

        const_cast<PageStorage&>(engine.storage()).storePage(std::move(page));
    }

    return Result<void>::ok();
}

} // namespace crawler
