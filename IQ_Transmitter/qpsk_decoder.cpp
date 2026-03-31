#include "qpsk_decoder.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace iqd {

namespace {

std::pair<uint8_t, uint8_t> DecodeQpskSymbol(float i, float q, double* outDistance) {
    constexpr float invSqrt2 = 0.70710678118F;

    struct Candidate {
        float i;
        float q;
        uint8_t b0;
        uint8_t b1;
    };

    static constexpr std::array<Candidate, 4> candidates = {{{invSqrt2, invSqrt2, 0U, 0U},
                                                              {-invSqrt2, invSqrt2, 0U, 1U},
                                                              {-invSqrt2, -invSqrt2, 1U, 1U},
                                                              {invSqrt2, -invSqrt2, 1U, 0U}}};

    double bestDist = std::numeric_limits<double>::max();
    uint8_t bestB0 = 0;
    uint8_t bestB1 = 0;

    for (const Candidate& c : candidates) {
        const double di = static_cast<double>(i) - static_cast<double>(c.i);
        const double dq = static_cast<double>(q) - static_cast<double>(c.q);
        const double d = std::sqrt(di * di + dq * dq);
        if (d < bestDist) {
            bestDist = d;
            bestB0 = c.b0;
            bestB1 = c.b1;
        }
    }

    if (outDistance != nullptr) {
        *outDistance = bestDist;
    }

    return {bestB0, bestB1};
}

std::vector<uint8_t> PackBitsMsbFirst(const std::vector<uint8_t>& bits) {
    std::vector<uint8_t> out;
    out.reserve((bits.size() + 7U) / 8U);

    size_t bitIndex = 0;
    while (bitIndex < bits.size()) {
        uint8_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value <<= 1U;
            if (bitIndex < bits.size()) {
                value |= (bits[bitIndex] & 0x01U);
                ++bitIndex;
            }
        }
        out.push_back(value);
    }

    return out;
}

} // namespace

DecodeResult DecodeIqQpskStream(const std::vector<float>& interleavedIq) {
    if (interleavedIq.size() % 2U != 0U) {
        throw std::runtime_error("IQ sample buffer must contain an even number of scalar values");
    }

    DecodeResult result;
    const size_t nComplex = interleavedIq.size() / 2U;
    result.bits.reserve(nComplex * 2U);

    double iSum = 0.0;
    double qSum = 0.0;
    double iPwr = 0.0;
    double qPwr = 0.0;
    double dSum = 0.0;

    for (size_t k = 0; k < interleavedIq.size(); k += 2U) {
        const float i = interleavedIq[k];
        const float q = interleavedIq[k + 1U];

        iSum += i;
        qSum += q;
        iPwr += static_cast<double>(i) * static_cast<double>(i);
        qPwr += static_cast<double>(q) * static_cast<double>(q);

        double distance = 0.0;
        const auto bits = DecodeQpskSymbol(i, q, &distance);
        dSum += distance;

        result.bits.push_back(bits.first);
        result.bits.push_back(bits.second);
    }

    result.bytes = PackBitsMsbFirst(result.bits);

    result.stats.complexSamples = nComplex;
    result.stats.recoveredBits = result.bits.size();
    result.stats.recoveredBytes = result.bytes.size();
    if (nComplex > 0U) {
        result.stats.iMean = iSum / static_cast<double>(nComplex);
        result.stats.qMean = qSum / static_cast<double>(nComplex);
        result.stats.iRms = std::sqrt(iPwr / static_cast<double>(nComplex));
        result.stats.qRms = std::sqrt(qPwr / static_cast<double>(nComplex));
        result.stats.meanNearestQpskDistance = dSum / static_cast<double>(nComplex);
    }

    return result;
}

bool RunSelfTest() {
    // Deterministic seed keeps self-test reproducible across runs.
    std::mt19937 rng(42U);
    std::uniform_int_distribution<int> d(0, 1);

    std::vector<uint8_t> refBits;
    refBits.reserve(20000U);

    for (size_t i = 0; i < 20000U; ++i) {
        refBits.push_back(static_cast<uint8_t>(d(rng)));
    }

    constexpr float invSqrt2 = 0.70710678118F;
    std::vector<float> iq;
    iq.reserve(refBits.size());

    for (size_t i = 0; i < refBits.size(); i += 2U) {
        const uint8_t b0 = refBits[i];
        const uint8_t b1 = (i + 1U < refBits.size()) ? refBits[i + 1U] : 0U;

        float iSample = invSqrt2;
        float qSample = invSqrt2;

        if (b0 == 0U && b1 == 0U) { iSample = invSqrt2; qSample = invSqrt2; }
        if (b0 == 0U && b1 == 1U) { iSample = -invSqrt2; qSample = invSqrt2; }
        if (b0 == 1U && b1 == 1U) { iSample = -invSqrt2; qSample = -invSqrt2; }
        if (b0 == 1U && b1 == 0U) { iSample = invSqrt2; qSample = -invSqrt2; }

        iq.push_back(iSample);
        iq.push_back(qSample);
    }

    const DecodeResult decoded = DecodeIqQpskStream(iq);
    if (decoded.bits.size() != refBits.size()) {
        return false;
    }

    for (size_t i = 0; i < refBits.size(); ++i) {
        if (decoded.bits[i] != refBits[i]) {
            return false;
        }
    }

    return true;
}

} // namespace iqd

