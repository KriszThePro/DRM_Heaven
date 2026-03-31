#pragma once

#include <cstdint>
#include <vector>

#include "drm_stage_interfaces.h"

namespace iqd {

DrmPipelineResult RunDrmTxSkeleton(const std::vector<uint8_t>& payloadBytes,
                                   const DrmPipelineConfig& cfg);

} // namespace iqd

