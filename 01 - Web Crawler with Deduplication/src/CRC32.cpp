#include "CRC32.h"

namespace crawler {

CRC32::CRC32() {
    initTable();
}

void CRC32::initTable() noexcept {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
        m_table[i] = crc;
    }
}

uint32_t CRC32::calculate(const uint8_t* data, size_t length) const noexcept {
    return update(0xFFFFFFFFU, data, length) ^ 0xFFFFFFFFU;
}

uint32_t CRC32::calculate(std::string_view sv) const noexcept {
    return calculate(reinterpret_cast<const uint8_t*>(sv.data()), sv.size());
}

uint32_t CRC32::calculate(const void* data, size_t length) const noexcept {
    return calculate(reinterpret_cast<const uint8_t*>(data), length);
}

uint32_t CRC32::update(uint32_t currentCrc, const uint8_t* data, size_t length) const noexcept {
    uint32_t crc = currentCrc;
    for (size_t i = 0; i < length; ++i) {
        crc = (crc >> 8) ^ m_table[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

const CRC32& getCRC32Instance() noexcept {
    static const CRC32 instance;
    return instance;
}

} // namespace crawler

