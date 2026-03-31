#pragma once

#include <cstdint>
#include <vector>

#include "iq_decoder_types.h"

namespace iqd {

DecodeResult DecodeIqQpskStream(const std::vector<float>& interleavedIq);
bool RunSelfTest();

} // namespace iqd

