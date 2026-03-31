#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iqd {

std::vector<uint8_t> ConvertIqToFl2kU8(const std::vector<float>& interleavedIq);
void StreamBytesToCommandStdin(const std::string& command, const std::vector<uint8_t>& bytes);

} // namespace iqd

