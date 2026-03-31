#pragma once

#include <cstdint>
#include <vector>

namespace iqd {

std::vector<uint8_t> DrmInterleaveBytes(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> DrmDeinterleaveBytes(const std::vector<uint8_t>& bytes);
bool RunDrmInterleaverSelfTest();

} // namespace iqd

