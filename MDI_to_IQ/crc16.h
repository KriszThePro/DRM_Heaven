#pragma once

#include <cstddef>
#include <cstdint>

namespace mdi {

uint16_t ComputeDreamCrc16(const uint8_t* bytes, size_t len);

} // namespace mdi

