#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iqd {

std::vector<float> ReadInterleavedIqAsFloat(const std::string& path, bool inputS16);
void WriteAllBytes(const std::string& path, const std::vector<uint8_t>& bytes);
void WriteTextFile(const std::string& path, const std::string& text);

} // namespace iqd

