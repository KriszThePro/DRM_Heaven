#pragma once

#include <cstddef>
#include <string>

#include "mdi_types.h"

namespace mdi {

void WriteIqMetaFile(const std::string& iqPath,
                     bool outputS16,
                     int sampleRateHz,
                     size_t scalarSamples,
                     const DecodeResult& decodeStats);

} // namespace mdi

