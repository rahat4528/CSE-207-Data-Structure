#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string_view>

namespace ttkv {

class CRC32 {
public:
    CRC32();

    void update(const void* data, size_t length);
    void update(std::string_view sv);
    uint32_t get() const;
    void reset();

    static uint32_t calculate(const void* data, size_t length);
    static uint32_t calculate(std::string_view sv);

private:
    uint32_t m_crc;
    static const uint32_t s_table[256];
};

} // namespace ttkv
