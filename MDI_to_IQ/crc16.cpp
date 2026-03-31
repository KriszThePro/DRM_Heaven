#include "crc16.h"

namespace mdi {

uint16_t ComputeDreamCrc16(const uint8_t* bytes, size_t len) {
    constexpr uint32_t bitOutPosMask = (1U << 16U);
    constexpr uint32_t polyMask = (1U << 12U) | (1U << 5U);
    uint32_t state = 0xFFFFFFFFU;

    for (size_t i = 0; i < len; ++i) {
        const uint8_t by = bytes[i];
        for (int b = 0; b < 8; ++b) {
            state <<= 1U;
            if ((state & bitOutPosMask) != 0U) {
                state = 1U;
            }
            if ((by & (1U << (7 - b))) != 0U) {
                state ^= 1U;
            }
            if ((state & 1U) != 0U) {
                state ^= polyMask;
            }
        }
    }

    state = ~state;
    return static_cast<uint16_t>(state & 0xFFFFU);
}

} // namespace mdi

