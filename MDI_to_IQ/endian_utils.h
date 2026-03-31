#pragma once

#include <cstdint>
#include <vector>

namespace mdi {

inline uint32_t ReadBe32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24U) |
           (static_cast<uint32_t>(p[1]) << 16U) |
           (static_cast<uint32_t>(p[2]) << 8U) |
           static_cast<uint32_t>(p[3]);
}

inline uint16_t ReadBe16(const uint8_t* p) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8U) | p[1]);
}

inline void AppendBe32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
}

inline void AppendBe16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
}

} // namespace mdi

