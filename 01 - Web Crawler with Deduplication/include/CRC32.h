#ifndef CRAWLER_CRC32_H
#define CRAWLER_CRC32_H

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <span>

namespace crawler {

class CRC32 {
public:
    CRC32();

    uint32_t calculate(const uint8_t* data, size_t length) const noexcept;
    uint32_t calculate(std::string_view sv) const noexcept;
    uint32_t calculate(const void* data, size_t length) const noexcept;

    // Incremental calculation
    uint32_t update(uint32_t currentCrc, const uint8_t* data, size_t length) const noexcept;

private:
    uint32_t m_table[256];
    void initTable() noexcept;
};

// Global helper singleton
const CRC32& getCRC32Instance() noexcept;

} // namespace crawler

#endif // CRAWLER_CRC32_H

