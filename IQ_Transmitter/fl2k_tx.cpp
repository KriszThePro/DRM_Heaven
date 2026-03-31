#include "fl2k_tx.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace iqd {

namespace {

constexpr float kFloatEps = 1.0e-6F;

TxConditioningTelemetry BuildTelemetryDefault() {
    TxConditioningTelemetry t;
    return t;
}

float LinearGainFromConfig(const TxConditioningConfig& cfg) {
    const float headroomLin = std::pow(10.0F, -cfg.headroomDb / 20.0F);
    return cfg.scale * headroomLin;
}

bool VerifyDeterministicConditioningVector() {
    const std::vector<float> in = {-1.20F, -0.50F, 0.00F, 0.50F, 1.20F, 0.25F};

    TxConditioningConfig cfg;
    cfg.scale = 1.0F;
    cfg.headroomDb = 0.0F;
    cfg.clip = true;
    cfg.shape = false;

    TxConditioningTelemetry telem;
    const std::vector<float> out = ApplyTxConditioning(in, cfg, &telem);

    if (out.size() != in.size()) {
        return false;
    }
    if (telem.overrunSamples != 2U || telem.clippedSamples != 2U || telem.scalarSamples != in.size()) {
        return false;
    }
    if (!(std::abs(out[0] + 1.0F) < kFloatEps
          && std::abs(out[2]) < kFloatEps
          && std::abs(out[4] - 1.0F) < kFloatEps
          && std::abs(out[5] - 0.25F) < kFloatEps)) {
        return false;
    }
    return true;
}

bool VerifyHeadroomAndU8Conversion() {
    const std::vector<float> in = {-1.0F, 0.0F, 1.0F, 0.5F};

    TxConditioningConfig cfg;
    cfg.scale = 1.0F;
    cfg.headroomDb = 6.0F;
    cfg.clip = false;
    cfg.shape = false;

    TxConditioningTelemetry telem;
    const std::vector<uint8_t> u8 = ConvertIqToFl2kU8(in, cfg, &telem);
    if (u8.size() != in.size()) {
        return false;
    }

    if (telem.overrunSamples != 0U || telem.clippedSamples != 0U) {
        return false;
    }

    return (u8[0] >= 62U && u8[0] <= 65U)
        && (u8[1] >= 126U && u8[1] <= 129U)
        && (u8[2] >= 189U && u8[2] <= 193U);
}

} // namespace

std::vector<float> ApplyTxConditioning(const std::vector<float>& interleavedIq,
                                       const TxConditioningConfig& cfg,
                                       TxConditioningTelemetry* telemetry) {
    if (interleavedIq.size() % 2U != 0U) {
        throw std::runtime_error("IQ sample buffer must contain an even number of scalar values");
    }

    TxConditioningTelemetry local = BuildTelemetryDefault();
    local.scalarSamples = interleavedIq.size();

    std::vector<float> out;
    out.reserve(interleavedIq.size());

    const float gain = LinearGainFromConfig(cfg);
    size_t zeroRun = 0U;

    for (float v : interleavedIq) {
        float conditioned = v * gain;
        const float absBefore = std::abs(conditioned);
        local.peakBefore = std::max(local.peakBefore, absBefore);

        if (absBefore > 1.0F) {
            ++local.overrunSamples;
        }

        if (cfg.shape) {
            conditioned = std::tanh(conditioned);
        }

        if (cfg.clip) {
            const float clipped = std::max(-1.0F, std::min(1.0F, conditioned));
            if (std::abs(clipped - conditioned) > kFloatEps) {
                ++local.clippedSamples;
            }
            conditioned = clipped;
        }

        const float absAfter = std::abs(conditioned);
        local.peakAfter = std::max(local.peakAfter, absAfter);

        if (absAfter < 1.0e-5F) {
            ++zeroRun;
            if (zeroRun >= 1024U) {
                ++local.underrunEvents;
                zeroRun = 0U;
            }
        } else {
            zeroRun = 0U;
        }

        out.push_back(conditioned);
    }

    if (telemetry != nullptr) {
        *telemetry = local;
    }

    return out;
}

std::vector<uint8_t> ConvertIqToFl2kU8(const std::vector<float>& interleavedIq,
                                       const TxConditioningConfig& cfg,
                                       TxConditioningTelemetry* telemetry) {
    const std::vector<float> conditioned = ApplyTxConditioning(interleavedIq, cfg, telemetry);

    std::vector<uint8_t> out;
    out.reserve(conditioned.size());

    for (float v : conditioned) {
        const float clamped = std::max(-1.0F, std::min(1.0F, v));
        const float scaled = clamped * 127.0F + 127.5F;
        out.push_back(static_cast<uint8_t>(scaled));
    }

    return out;
}

std::vector<uint8_t> ConvertIqToFl2kU8(const std::vector<float>& interleavedIq) {
    const TxConditioningConfig cfg;
    return ConvertIqToFl2kU8(interleavedIq, cfg, nullptr);
}

void StreamBytesToCommandStdin(const std::string& command, const std::vector<uint8_t>& bytes) {
    FILE* pipe = _popen(command.c_str(), "wb");
    if (pipe == nullptr) {
        throw std::runtime_error("Failed to start external command for FL2K streaming");
    }

    const size_t written = fwrite(bytes.data(), 1U, bytes.size(), pipe);
    if (written != bytes.size()) {
        _pclose(pipe);
        throw std::runtime_error("Failed while writing FL2K byte stream to command stdin");
    }

    const int rc = _pclose(pipe);
    if (rc != 0) {
        throw std::runtime_error("External FL2K command exited with non-zero status");
    }
}

bool RunTxConditioningSelfTest() {
    return VerifyDeterministicConditioningVector() && VerifyHeadroomAndU8Conversion();
}

} // namespace iqd

