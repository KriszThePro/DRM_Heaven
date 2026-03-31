#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace iqd {

struct DrmOfdmConfig {
    size_t fftSize = 64U;
    size_t guardSamples = 16U;
};

std::vector<float> BuildDrmOfdmIq(const std::vector<uint8_t>& framedBytes,
                                  const DrmOfdmConfig& cfg);

size_t EstimateDrmOfdmSymbolCount(const std::vector<uint8_t>& framedBytes,
                                  const DrmOfdmConfig& cfg);

bool RunDrmOfdmSelfTest();

} // namespace iqd

