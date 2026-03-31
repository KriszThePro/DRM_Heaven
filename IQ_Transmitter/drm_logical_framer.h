#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace iqd {

struct DrmLogicalFrame {
    uint16_t frameIndex = 0U;
    uint16_t totalFrames = 0U;
    uint8_t serviceId = 0U;
    std::vector<uint8_t> facBytes;
    std::vector<uint8_t> sdcBytes;
    std::vector<uint8_t> mscBytes;
    std::vector<uint8_t> packedBytes;
};

std::vector<DrmLogicalFrame> AssembleDrmLogicalFrames(const std::vector<uint8_t>& interleavedBytes,
                                                      uint8_t serviceId,
                                                      size_t mscBytesPerFrame);

std::vector<uint8_t> PackDrmLogicalFrames(const std::vector<DrmLogicalFrame>& frames);

bool RunDrmLogicalFramerSelfTest();

} // namespace iqd

