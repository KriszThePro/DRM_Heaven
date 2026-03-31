#pragma once

#include <string>
#include <vector>

#include "drm_stage_interfaces.h"

namespace iqd {

bool ApplyDrmProfile(const std::string& profileName, DrmPipelineConfig* cfg);
std::vector<std::string> ListDrmProfiles();

} // namespace iqd

