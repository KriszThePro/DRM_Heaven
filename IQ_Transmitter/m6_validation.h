#pragma once

#include <string>

#include "drm_stage_interfaces.h"
#include "iq_decoder_types.h"

namespace iqd {

struct M6KpiReport {
    std::string runId;
    std::string labTag;
    std::string profile;
    size_t payloadBytes = 0U;
    size_t logicalFrames = 0U;
    size_t ofdmSymbols = 0U;
    double meanNearestQpskDistance = 0.0;
    double txPeakAfter = 0.0;
    size_t txOverrunSamples = 0U;
    size_t txUnderrunEvents = 0U;
    bool lockProxy = false;
    bool decodeProxy = false;
};

M6KpiReport BuildM6KpiReport(const DecodeResult& decoded,
                             const DrmPipelineResult& tx,
                             const std::string& profile,
                             const std::string& runId,
                             const std::string& labTag);

bool ValidateM6KpiReport(const M6KpiReport& report, std::string* reason);
std::string SerializeM6KpiReport(const M6KpiReport& report);
bool RunM6SelfTest();

} // namespace iqd

