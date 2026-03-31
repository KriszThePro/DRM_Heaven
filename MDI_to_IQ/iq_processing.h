#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mdi_types.h"

namespace mdi {

std::vector<float> ConvertFramesToIq(const std::vector<MdiFrame>& frames);
void WriteIqF32(const std::string& path, const std::vector<float>& iq);
void WriteIqS16(const std::string& path, const std::vector<float>& iq);

} // namespace mdi

