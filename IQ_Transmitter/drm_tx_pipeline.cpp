#include "drm_tx_pipeline.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "file_io.h"

namespace iqd {

namespace {

std::vector<uint8_t> StageChannelCoder(const std::vector<uint8_t>& in) {
    // M1 skeleton: pass-through placeholder for DRM channel coding.
    return in;
}

std::vector<uint8_t> StageInterleaver(const std::vector<uint8_t>& in) {
    // M1 skeleton: pass-through placeholder for DRM interleaving.
    return in;
}

std::vector<uint8_t> StageFrameBuilder(const std::vector<uint8_t>& in) {
    // M1 skeleton: pass-through placeholder for FAC/SDC/MSC framing.
    return in;
}

std::vector<float> StageOfdmMapper(const std::vector<uint8_t>& framedBytes) {
    // M1 skeleton: use deterministic QPSK mapping as a temporary OFDM placeholder.
    constexpr float invSqrt2 = 0.70710678118F;
    std::vector<float> iq;
    iq.reserve(framedBytes.size() * 8U);

    for (uint8_t by : framedBytes) {
        for (int bit = 7; bit >= 0; bit -= 2) {
            const uint8_t b0 = static_cast<uint8_t>((by >> bit) & 0x01U);
            const uint8_t b1 = static_cast<uint8_t>((by >> (bit - 1)) & 0x01U);

            float iSample = invSqrt2;
            float qSample = invSqrt2;

            if (b0 == 0U && b1 == 0U) { iSample = invSqrt2; qSample = invSqrt2; }
            if (b0 == 0U && b1 == 1U) { iSample = -invSqrt2; qSample = invSqrt2; }
            if (b0 == 1U && b1 == 1U) { iSample = -invSqrt2; qSample = -invSqrt2; }
            if (b0 == 1U && b1 == 0U) { iSample = invSqrt2; qSample = -invSqrt2; }

            iq.push_back(iSample);
            iq.push_back(qSample);
        }
    }

    return iq;
}

std::vector<uint8_t> FloatToBytes(const std::vector<float>& values) {
    std::vector<uint8_t> out;
    out.resize(values.size() * sizeof(float));
    const uint8_t* p = reinterpret_cast<const uint8_t*>(values.data());
    std::copy(p, p + out.size(), out.begin());
    return out;
}

void DumpStageIfNeeded(const DrmPipelineConfig& cfg,
                       const std::string& fileName,
                       const std::vector<uint8_t>& data) {
    if (!cfg.dumpStages) {
        return;
    }
    const std::string prefix = cfg.dumpDir.empty() ? std::string(".") : cfg.dumpDir;
    const std::string path = prefix + "\\" + fileName;
    WriteAllBytes(path, data);
}

} // namespace

DrmPipelineResult RunDrmTxSkeleton(const std::vector<uint8_t>& payloadBytes,
                                   const DrmPipelineConfig& cfg) {
    if (payloadBytes.empty()) {
        throw std::runtime_error("DRM TX skeleton received empty payload");
    }

    DrmPipelineResult out;
    out.codedBytes = StageChannelCoder(payloadBytes);
    DumpStageIfNeeded(cfg, "m1_stage_coded.bin", out.codedBytes);

    out.interleavedBytes = StageInterleaver(out.codedBytes);
    DumpStageIfNeeded(cfg, "m1_stage_interleaved.bin", out.interleavedBytes);

    out.framedBytes = StageFrameBuilder(out.interleavedBytes);
    DumpStageIfNeeded(cfg, "m1_stage_framed.bin", out.framedBytes);

    out.ofdmIqF32Interleaved = StageOfdmMapper(out.framedBytes);
    DumpStageIfNeeded(cfg, "m1_stage_ofdm_iq_f32.bin", FloatToBytes(out.ofdmIqF32Interleaved));

    return out;
}

} // namespace iqd

