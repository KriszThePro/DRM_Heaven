#pragma once

#include <cstdint>
#include <vector>

#include "mdi_types.h"

namespace mdi {

DecodeResult DecodeMdiAfPackets(const std::vector<uint8_t>& fileBytes, bool strictCrc);

} // namespace mdi

