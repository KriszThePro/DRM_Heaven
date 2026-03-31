#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iqd {

struct DrmPipelineConfig {
    bool dumpStages = false;
    std::string dumpDir;
};

struct DrmPipelineResult {
    std::vector<uint8_t> codedBytes;
    std::vector<uint8_t> interleavedBytes;
    std::vector<uint8_t> framedBytes;
    std::vector<float> ofdmIqF32Interleaved;
};

} // namespace iqd

