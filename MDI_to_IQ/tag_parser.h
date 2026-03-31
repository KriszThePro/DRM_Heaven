#pragma once

#include <cstdint>
#include <vector>

#include "mdi_types.h"

namespace mdi {

void ParseTagPayload(const std::vector<uint8_t>& payload, MdiFrame& frame);

} // namespace mdi

