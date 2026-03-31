#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mdi {

std::vector<uint8_t> ReadAllBytes(const std::string& path);
void WriteAllBytes(const std::string& path, const std::vector<uint8_t>& bytes);
void WriteTextFile(const std::string& path, const std::string& text);

} // namespace mdi

