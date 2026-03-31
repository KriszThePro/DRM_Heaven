#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace iqd {

struct DrmPipelineConfig {
    bool dumpStages = false;
    std::string dumpDir;
    uint8_t serviceId = 1U;
    size_t mscBytesPerFrame = 192U;
    size_t ofdmFftSize = 64U;
    size_t ofdmGuardSamples = 16U;
    float txScale = 1.0F;
    float txHeadroomDb = 0.0F;
    bool txClip = false;
    bool txShape = false;
};

struct DrmPipelineResult {
    std::vector<uint8_t> codedBytes;
    std::vector<uint8_t> interleavedBytes;
    std::vector<uint8_t> framedBytes;
    size_t logicalFrameCount = 0U;
    size_t ofdmSymbolCount = 0U;
    size_t txOverrunSamples = 0U;
    size_t txClippedSamples = 0U;
    size_t txUnderrunEvents = 0U;
    float txPeakBefore = 0.0F;
    float txPeakAfter = 0.0F;
    std::vector<float> ofdmIqF32Interleaved;
};

} // namespace iqd

