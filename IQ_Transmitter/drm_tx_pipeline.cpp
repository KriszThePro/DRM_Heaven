#include "drm_tx_pipeline.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "drm_coder.h"
#include "drm_interleaver.h"
#include "drm_logical_framer.h"
#include "drm_ofdm_modulator.h"
#include "file_io.h"
#include "fl2k_tx.h"

namespace iqd {

namespace {

std::vector<uint8_t> StageChannelCoder(const std::vector<uint8_t>& in) {
    return DrmEncodePayload(in);
}

std::vector<uint8_t> StageInterleaver(const std::vector<uint8_t>& in) {
    return DrmInterleaveBytes(in);
}

std::vector<float> StageOfdmMapper(const std::vector<uint8_t>& framedBytes,
                                   const DrmPipelineConfig& cfg,
                                   size_t* outSymbolCount,
                                   TxConditioningTelemetry* telemetry) {
    DrmOfdmConfig ofdmCfg;
    ofdmCfg.fftSize = cfg.ofdmFftSize;
    ofdmCfg.guardSamples = cfg.ofdmGuardSamples;

    if (outSymbolCount != nullptr) {
        *outSymbolCount = EstimateDrmOfdmSymbolCount(framedBytes, ofdmCfg);
    }

    const std::vector<float> raw = BuildDrmOfdmIq(framedBytes, ofdmCfg);

    TxConditioningConfig txCfg;
    txCfg.scale = cfg.txScale;
    txCfg.headroomDb = cfg.txHeadroomDb;
    txCfg.clip = cfg.txClip;
    txCfg.shape = cfg.txShape;
    return ApplyTxConditioning(raw, txCfg, telemetry);
}

void DumpLogicalFramesIfNeeded(const DrmPipelineConfig& cfg,
                               const std::vector<DrmLogicalFrame>& frames) {
    if (!cfg.dumpStages) {
        return;
    }

    const std::string prefix = cfg.dumpDir.empty() ? std::string(".") : cfg.dumpDir;
    for (size_t i = 0; i < frames.size(); ++i) {
        std::ostringstream oss;
        oss << "drm_frame_" << std::setfill('0') << std::setw(4) << i;
        WriteAllBytes(prefix + "\\" + oss.str() + "_fac.bin", frames[i].facBytes);
        WriteAllBytes(prefix + "\\" + oss.str() + "_sdc.bin", frames[i].sdcBytes);
        WriteAllBytes(prefix + "\\" + oss.str() + "_msc.bin", frames[i].mscBytes);
        WriteAllBytes(prefix + "\\" + oss.str() + "_packed.bin", frames[i].packedBytes);
    }
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
    DumpStageIfNeeded(cfg, "drm_stage_coded.bin", out.codedBytes);

    out.interleavedBytes = StageInterleaver(out.codedBytes);
    DumpStageIfNeeded(cfg, "drm_stage_interleaved.bin", out.interleavedBytes);

    const std::vector<DrmLogicalFrame> frames = AssembleDrmLogicalFrames(out.interleavedBytes,
                                                                          cfg.serviceId,
                                                                          cfg.mscBytesPerFrame);
    out.logicalFrameCount = frames.size();
    DumpLogicalFramesIfNeeded(cfg, frames);

    out.framedBytes = PackDrmLogicalFrames(frames);
    DumpStageIfNeeded(cfg, "drm_stage_framed.bin", out.framedBytes);

    TxConditioningTelemetry telem;
    out.ofdmIqF32Interleaved = StageOfdmMapper(out.framedBytes, cfg, &out.ofdmSymbolCount, &telem);
    out.txOverrunSamples = telem.overrunSamples;
    out.txClippedSamples = telem.clippedSamples;
    out.txUnderrunEvents = telem.underrunEvents;
    out.txPeakBefore = telem.peakBefore;
    out.txPeakAfter = telem.peakAfter;
    DumpStageIfNeeded(cfg, "drm_stage_ofdm_iq_f32.bin", FloatToBytes(out.ofdmIqF32Interleaved));

    return out;
}

} // namespace iqd

