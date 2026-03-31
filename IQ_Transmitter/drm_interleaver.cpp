#include "drm_interleaver.h"

#include <array>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

namespace iqd {

namespace {

size_t ChooseCoprimeStep(size_t n) {
    if (n <= 1U) {
        return 1U;
    }

    static constexpr std::array<size_t, 6> preferred = {17U, 13U, 11U, 7U, 5U, 3U};
    for (size_t candidate : preferred) {
        const size_t step = candidate % n;
        if (step != 0U && std::gcd(step, n) == 1U) {
            return step;
        }
    }

    for (size_t step = 2U; step < n; ++step) {
        if (std::gcd(step, n) == 1U) {
            return step;
        }
    }

    return 1U;
}

size_t ComputeOffset(size_t n) {
    return (n == 0U) ? 0U : (3U % n);
}

std::vector<uint8_t> InterleaveInternal(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return {};
    }

    const size_t n = bytes.size();
    const size_t step = ChooseCoprimeStep(n);
    const size_t offset = ComputeOffset(n);

    std::vector<uint8_t> out(n, 0U);
    for (size_t i = 0; i < n; ++i) {
        const size_t dst = (i * step + offset) % n;
        out[dst] = bytes[i];
    }

    return out;
}

std::vector<uint8_t> DeinterleaveInternal(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return {};
    }

    const size_t n = bytes.size();
    const size_t step = ChooseCoprimeStep(n);
    const size_t offset = ComputeOffset(n);

    std::vector<uint8_t> out(n, 0U);
    for (size_t i = 0; i < n; ++i) {
        const size_t src = (i * step + offset) % n;
        out[i] = bytes[src];
    }

    return out;
}

bool VerifyDeterministicVector() {
    static const std::vector<uint8_t> input = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    static const std::vector<uint8_t> expectedInterleaved = {1U, 4U, 7U, 0U, 3U, 6U, 9U, 2U, 5U, 8U};

    const std::vector<uint8_t> interleaved = DrmInterleaveBytes(input);
    if (interleaved != expectedInterleaved) {
        return false;
    }

    const std::vector<uint8_t> deinterleaved = DrmDeinterleaveBytes(interleaved);
    return deinterleaved == input;
}

bool VerifySeededRoundTrip() {
    std::mt19937 rng(20260331U);
    std::uniform_int_distribution<int> byteDist(0, 255);

    for (size_t n = 1U; n <= 257U; ++n) {
        std::vector<uint8_t> data;
        data.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            data.push_back(static_cast<uint8_t>(byteDist(rng)));
        }

        const std::vector<uint8_t> interleaved = DrmInterleaveBytes(data);
        const std::vector<uint8_t> restored = DrmDeinterleaveBytes(interleaved);
        if (restored != data) {
            return false;
        }
    }

    return true;
}

} // namespace

std::vector<uint8_t> DrmInterleaveBytes(const std::vector<uint8_t>& bytes) {
    return InterleaveInternal(bytes);
}

std::vector<uint8_t> DrmDeinterleaveBytes(const std::vector<uint8_t>& bytes) {
    return DeinterleaveInternal(bytes);
}

bool RunDrmInterleaverSelfTest() {
    return VerifyDeterministicVector()
        && DrmInterleaveBytes({}).empty()
        && DrmDeinterleaveBytes({}).empty()
        && VerifySeededRoundTrip();
}

} // namespace iqd

