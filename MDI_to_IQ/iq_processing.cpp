#include "iq_processing.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace mdi {

std::vector<float> ConvertFramesToIq(const std::vector<MdiFrame>& frames) {
    constexpr float invSqrt2 = 0.70710678118F;
    std::vector<float> iq;

    for (const MdiFrame& frame : frames) {
        std::vector<uint8_t> bitstream;
        bitstream.reserve(frame.facBits.size() + frame.sdcBits.size() +
                          frame.streamBits[0].size() + frame.streamBits[1].size() +
                          frame.streamBits[2].size() + frame.streamBits[3].size());

        bitstream.insert(bitstream.end(), frame.facBits.begin(), frame.facBits.end());
        bitstream.insert(bitstream.end(), frame.sdcBits.begin(), frame.sdcBits.end());
        for (const auto& stream : frame.streamBits) {
            bitstream.insert(bitstream.end(), stream.begin(), stream.end());
        }

        // Practical mapping stage: QPSK symbolization of decoded MDI payload bits.
        for (size_t i = 0; i < bitstream.size(); i += 2) {
            const uint8_t b0 = bitstream[i];
            const uint8_t b1 = (i + 1 < bitstream.size()) ? bitstream[i + 1] : 0U;

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

void WriteIqF32(const std::string& path, const std::vector<float>& iq) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open output file: " + path);
    }

    out.write(reinterpret_cast<const char*>(iq.data()), static_cast<std::streamsize>(iq.size() * sizeof(float)));
    if (!out) {
        throw std::runtime_error("Failed while writing IQ float32 output");
    }
}

void WriteIqS16(const std::string& path, const std::vector<float>& iq) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open output file: " + path);
    }

    std::vector<int16_t> packed;
    packed.reserve(iq.size());
    for (float v : iq) {
        const float clamped = std::max(-1.0F, std::min(1.0F, v));
        const float scaled = clamped * 32767.0F;
        packed.push_back(static_cast<int16_t>(std::lrint(scaled)));
    }

    out.write(reinterpret_cast<const char*>(packed.data()), static_cast<std::streamsize>(packed.size() * sizeof(int16_t)));
    if (!out) {
        throw std::runtime_error("Failed while writing IQ int16 output");
    }
}

} // namespace mdi

