#include "drm_profiles.h"

#include <string>
#include <vector>

namespace iqd {

bool ApplyDrmProfile(const std::string& profileName, DrmPipelineConfig* cfg) {
    if (cfg == nullptr) {
        return false;
    }

    if (profileName == "robust") {
        cfg->mscBytesPerFrame = 128U;
        cfg->ofdmFftSize = 64U;
        cfg->ofdmGuardSamples = 16U;
        cfg->txHeadroomDb = 6.0F;
        return true;
    }

    if (profileName == "balanced") {
        cfg->mscBytesPerFrame = 192U;
        cfg->ofdmFftSize = 64U;
        cfg->ofdmGuardSamples = 16U;
        cfg->txHeadroomDb = 3.0F;
        return true;
    }

    if (profileName == "highrate") {
        cfg->mscBytesPerFrame = 256U;
        cfg->ofdmFftSize = 128U;
        cfg->ofdmGuardSamples = 16U;
        cfg->txHeadroomDb = 1.5F;
        return true;
    }

    return false;
}

std::vector<std::string> ListDrmProfiles() {
    return {"robust", "balanced", "highrate"};
}

} // namespace iqd

