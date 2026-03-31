#pragma once

#include <cstdint>
#include <vector>

namespace iqd {

std::vector<uint8_t> DrmEncodePayload(const std::vector<uint8_t>& payloadBytes);
std::vector<uint8_t> DrmDecodePayload(const std::vector<uint8_t>& codedBytes);
bool RunDrmCoderSelfTest();

} // namespace iqd

