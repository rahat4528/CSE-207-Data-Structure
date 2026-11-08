#include "CRC32.h"

namespace ttkv {

namespace {
// Compile-time generated IEEE 802.3 CRC32 lookup table (polynomial 0xEDB88320)
constexpr auto generateCrc32Table() {
    struct Table {
        uint32_t data[256];
    } table{};

    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
        table.data[i] = crc;
    }
    return table;
}

constexpr auto kCrcTable = generateCrc32Table();
} // namespace

const uint32_t CRC32::s_table[256] = {
#define ROW(i) kCrcTable.data[i]
    ROW(0), ROW(1), ROW(2), ROW(3), ROW(4), ROW(5), ROW(6), ROW(7),
    ROW(8), ROW(9), ROW(10), ROW(11), ROW(12), ROW(13), ROW(14), ROW(15),
    ROW(16), ROW(17), ROW(18), ROW(19), ROW(20), ROW(21), ROW(22), ROW(23),
    ROW(24), ROW(25), ROW(26), ROW(27), ROW(28), ROW(29), ROW(30), ROW(31),
    ROW(32), ROW(33), ROW(34), ROW(35), ROW(36), ROW(37), ROW(38), ROW(39),
    ROW(40), ROW(41), ROW(42), ROW(43), ROW(44), ROW(45), ROW(46), ROW(47),
    ROW(48), ROW(49), ROW(50), ROW(51), ROW(52), ROW(53), ROW(54), ROW(55),
    ROW(56), ROW(57), ROW(58), ROW(59), ROW(60), ROW(61), ROW(62), ROW(63),
    ROW(64), ROW(65), ROW(66), ROW(67), ROW(68), ROW(69), ROW(70), ROW(71),
    ROW(72), ROW(73), ROW(74), ROW(75), ROW(76), ROW(77), ROW(78), ROW(79),
    ROW(80), ROW(81), ROW(82), ROW(83), ROW(84), ROW(85), ROW(86), ROW(87),
    ROW(88), ROW(89), ROW(90), ROW(91), ROW(92), ROW(93), ROW(94), ROW(95),
    ROW(96), ROW(97), ROW(98), ROW(99), ROW(100), ROW(101), ROW(102), ROW(103),
    ROW(104), ROW(105), ROW(106), ROW(107), ROW(108), ROW(109), ROW(110), ROW(111),
    ROW(112), ROW(113), ROW(114), ROW(115), ROW(116), ROW(117), ROW(118), ROW(119),
    ROW(120), ROW(121), ROW(122), ROW(123), ROW(124), ROW(125), ROW(126), ROW(127),
    ROW(128), ROW(129), ROW(130), ROW(131), ROW(132), ROW(133), ROW(134), ROW(135),
    ROW(136), ROW(137), ROW(138), ROW(139), ROW(140), ROW(141), ROW(142), ROW(143),
    ROW(144), ROW(145), ROW(146), ROW(147), ROW(148), ROW(149), ROW(150), ROW(151),
    ROW(152), ROW(153), ROW(154), ROW(155), ROW(156), ROW(157), ROW(158), ROW(159),
    ROW(160), ROW(161), ROW(162), ROW(163), ROW(164), ROW(165), ROW(166), ROW(167),
    ROW(168), ROW(169), ROW(170), ROW(171), ROW(172), ROW(173), ROW(174), ROW(175),
    ROW(176), ROW(177), ROW(178), ROW(179), ROW(180), ROW(181), ROW(182), ROW(183),
    ROW(184), ROW(185), ROW(186), ROW(187), ROW(188), ROW(189), ROW(190), ROW(191),
    ROW(192), ROW(193), ROW(194), ROW(195), ROW(196), ROW(197), ROW(198), ROW(199),
    ROW(200), ROW(201), ROW(202), ROW(203), ROW(204), ROW(205), ROW(206), ROW(207),
    ROW(208), ROW(209), ROW(210), ROW(211), ROW(212), ROW(213), ROW(214), ROW(215),
    ROW(216), ROW(217), ROW(218), ROW(219), ROW(220), ROW(221), ROW(222), ROW(223),
    ROW(224), ROW(225), ROW(226), ROW(227), ROW(228), ROW(229), ROW(230), ROW(231),
    ROW(232), ROW(233), ROW(234), ROW(235), ROW(236), ROW(237), ROW(238), ROW(239),
    ROW(240), ROW(241), ROW(242), ROW(243), ROW(244), ROW(245), ROW(246), ROW(247),
    ROW(248), ROW(249), ROW(250), ROW(251), ROW(252), ROW(253), ROW(254), ROW(255)
#undef ROW
};

CRC32::CRC32() : m_crc(0xFFFFFFFFu) {}

void CRC32::update(const void* data, size_t length) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < length; ++i) {
        uint8_t tableIndex = static_cast<uint8_t>((m_crc ^ bytes[i]) & 0xFF);
        m_crc = (m_crc >> 8) ^ s_table[tableIndex];
    }
}

void CRC32::update(std::string_view sv) {
    update(sv.data(), sv.size());
}

uint32_t CRC32::get() const {
    return m_crc ^ 0xFFFFFFFFu;
}

void CRC32::reset() {
    m_crc = 0xFFFFFFFFu;
}

uint32_t CRC32::calculate(const void* data, size_t length) {
    CRC32 crc;
    crc.update(data, length);
    return crc.get();
}

uint32_t CRC32::calculate(std::string_view sv) {
    return calculate(sv.data(), sv.size());
}

} // namespace ttkv
