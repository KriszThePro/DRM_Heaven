#include "drm_ofdm_modulator.h"

#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace iqd {

namespace {

constexpr float kInvSqrt2 = 0.70710678118F;

std::vector<int> BuildActiveCarriers() {
    std::vector<int> carriers;
    carriers.reserve(52U);
    for (int c = -26; c <= 26; ++c) {
        if (c != 0) {
            carriers.push_back(c);
        }
    }
    return carriers;
}

bool IsPilotCarrier(int carrier) {
    static constexpr std::array<int, 4> pilots = {-21, -7, 7, 21};
    for (int p : pilots) {
        if (carrier == p) {
            return true;
        }
    }
    return false;
}

std::vector<int> BuildDataCarriers() {
    const std::vector<int> active = BuildActiveCarriers();
    std::vector<int> data;
    data.reserve(active.size());
    for (int carrier : active) {
        if (!IsPilotCarrier(carrier)) {
            data.push_back(carrier);
        }
    }
    return data;
}

size_t CarrierToBin(size_t fftSize, int carrier) {
    if (carrier > 0) {
        return static_cast<size_t>(carrier);
    }
    return fftSize - static_cast<size_t>(-carrier);
}

std::complex<float> MapQpsk(uint8_t b0, uint8_t b1) {
    if (b0 == 0U && b1 == 0U) { return {kInvSqrt2, kInvSqrt2}; }
    if (b0 == 0U && b1 == 1U) { return {-kInvSqrt2, kInvSqrt2}; }
    if (b0 == 1U && b1 == 1U) { return {-kInvSqrt2, -kInvSqrt2}; }
    return {kInvSqrt2, -kInvSqrt2};
}

std::vector<uint8_t> BytesToBitsMsb(const std::vector<uint8_t>& bytes) {
    std::vector<uint8_t> bits;
    bits.reserve(bytes.size() * 8U);

    for (uint8_t by : bytes) {
        for (int i = 7; i >= 0; --i) {
            bits.push_back(static_cast<uint8_t>((by >> i) & 0x01U));
        }
    }

    return bits;
}

std::complex<float> PilotTone(size_t symbolIndex, size_t pilotIndex) {
    const bool invert = ((symbolIndex + pilotIndex * 3U) % 2U) != 0U;
    return invert ? std::complex<float>(-1.0F, 0.0F) : std::complex<float>(1.0F, 0.0F);
}

std::vector<std::complex<float>> BuildFrequencyBins(const std::vector<std::complex<float>>& dataSymbols,
                                                    size_t symbolIndex,
                                                    const DrmOfdmConfig& cfg) {
    if (cfg.fftSize < 64U) {
        throw std::runtime_error("OFDM FFT size must be at least 64");
    }

    const std::vector<int> dataCarriers = BuildDataCarriers();
    const std::vector<int> activeCarriers = BuildActiveCarriers();

    std::vector<std::complex<float>> bins(cfg.fftSize, {0.0F, 0.0F});

    size_t dataAt = 0U;
    size_t pilotAt = 0U;
    for (int carrier : activeCarriers) {
        const size_t bin = CarrierToBin(cfg.fftSize, carrier);
        if (IsPilotCarrier(carrier)) {
            bins[bin] = PilotTone(symbolIndex, pilotAt++);
        } else {
            bins[bin] = (dataAt < dataSymbols.size()) ? dataSymbols[dataAt] : std::complex<float>(0.0F, 0.0F);
            ++dataAt;
        }
    }

    if (dataSymbols.size() > dataCarriers.size()) {
        throw std::runtime_error("Too many data symbols for one OFDM symbol");
    }

    return bins;
}

std::vector<std::complex<float>> RunIfft(const std::vector<std::complex<float>>& bins) {
    const size_t n = bins.size();
    const float twoPi = 6.28318530718F;

    std::vector<std::complex<float>> time(n, {0.0F, 0.0F});

    for (size_t t = 0U; t < n; ++t) {
        std::complex<float> sum(0.0F, 0.0F);
        for (size_t k = 0U; k < n; ++k) {
            const float angle = twoPi * static_cast<float>(k * t) / static_cast<float>(n);
            const std::complex<float> w(std::cos(angle), std::sin(angle));
            sum += bins[k] * w;
        }
        time[t] = sum / static_cast<float>(n);
    }

    return time;
}

std::vector<std::complex<float>> AddGuard(const std::vector<std::complex<float>>& symbol,
                                          size_t guardSamples) {
    if (guardSamples >= symbol.size()) {
        throw std::runtime_error("OFDM guard interval must be smaller than FFT size");
    }

    std::vector<std::complex<float>> out;
    out.reserve(symbol.size() + guardSamples);

    out.insert(out.end(), symbol.end() - static_cast<std::ptrdiff_t>(guardSamples), symbol.end());
    out.insert(out.end(), symbol.begin(), symbol.end());
    return out;
}

uint64_t HashFloatsFnv1a(const std::vector<float>& values) {
    uint64_t hash = 1469598103934665603ULL;
    for (float value : values) {
        uint32_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(uint32_t));
        for (int i = 0; i < 4; ++i) {
            const uint8_t by = static_cast<uint8_t>((bits >> (i * 8)) & 0xFFU);
            hash ^= static_cast<uint64_t>(by);
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

bool VerifyCarrierAndPilotMap() {
    DrmOfdmConfig cfg;
    std::vector<std::complex<float>> data(48U, {kInvSqrt2, kInvSqrt2});
    const std::vector<std::complex<float>> bins = BuildFrequencyBins(data, 2U, cfg);

    if (bins.size() != cfg.fftSize || bins[0] != std::complex<float>(0.0F, 0.0F)) {
        return false;
    }

    if (bins[CarrierToBin(cfg.fftSize, -21)] != PilotTone(2U, 0U)) { return false; }
    if (bins[CarrierToBin(cfg.fftSize, -7)] != PilotTone(2U, 1U)) { return false; }
    if (bins[CarrierToBin(cfg.fftSize, 7)] != PilotTone(2U, 2U)) { return false; }
    if (bins[CarrierToBin(cfg.fftSize, 21)] != PilotTone(2U, 3U)) { return false; }

    return true;
}

bool VerifyDeterministicVector() {
    DrmOfdmConfig cfg;
    std::vector<uint8_t> input(24U, 0x5AU);

    const std::vector<float> iq = BuildDrmOfdmIq(input, cfg);
    const size_t expectedSymbols = 2U;
    const size_t expectedScalars = expectedSymbols * (cfg.fftSize + cfg.guardSamples) * 2U;
    if (iq.size() != expectedScalars) {
        return false;
    }

    const uint64_t hash = HashFloatsFnv1a(iq);
    return hash == 12878328325908408595ULL;
}

} // namespace

size_t EstimateDrmOfdmSymbolCount(const std::vector<uint8_t>& framedBytes,
                                  const DrmOfdmConfig& cfg) {
    if (framedBytes.empty()) {
        throw std::runtime_error("OFDM modulator received empty framed payload");
    }

    if (cfg.fftSize < 64U) {
        throw std::runtime_error("OFDM FFT size must be at least 64");
    }

    const size_t dataCarriers = BuildDataCarriers().size();
    const size_t bitsPerSymbol = dataCarriers * 2U;
    const size_t totalBits = framedBytes.size() * 8U;
    return (totalBits + bitsPerSymbol - 1U) / bitsPerSymbol;
}

std::vector<float> BuildDrmOfdmIq(const std::vector<uint8_t>& framedBytes,
                                  const DrmOfdmConfig& cfg) {
    const size_t nSymbols = EstimateDrmOfdmSymbolCount(framedBytes, cfg);

    const std::vector<int> dataCarriers = BuildDataCarriers();
    const size_t bitsPerSymbol = dataCarriers.size() * 2U;
    const std::vector<uint8_t> bits = BytesToBitsMsb(framedBytes);

    std::vector<float> out;
    out.reserve(nSymbols * (cfg.fftSize + cfg.guardSamples) * 2U);

    size_t bitAt = 0U;
    for (size_t symbolIdx = 0U; symbolIdx < nSymbols; ++symbolIdx) {
        std::vector<std::complex<float>> dataSymbols;
        dataSymbols.reserve(dataCarriers.size());

        for (size_t c = 0U; c < dataCarriers.size(); ++c) {
            const uint8_t b0 = (bitAt < bits.size()) ? bits[bitAt++] : 0U;
            const uint8_t b1 = (bitAt < bits.size()) ? bits[bitAt++] : 0U;
            dataSymbols.push_back(MapQpsk(b0, b1));
        }

        const std::vector<std::complex<float>> bins = BuildFrequencyBins(dataSymbols, symbolIdx, cfg);
        const std::vector<std::complex<float>> time = RunIfft(bins);
        const std::vector<std::complex<float>> withGuard = AddGuard(time, cfg.guardSamples);

        for (const std::complex<float>& sample : withGuard) {
            out.push_back(sample.real());
            out.push_back(sample.imag());
        }

        const size_t consumedForSymbol = dataCarriers.size() * 2U;
        if (consumedForSymbol > bitsPerSymbol) {
            throw std::runtime_error("Unexpected OFDM symbol bit accounting error");
        }
    }

    return out;
}

bool RunDrmOfdmSelfTest() {
    return VerifyCarrierAndPilotMap() && VerifyDeterministicVector();
}

} // namespace iqd

