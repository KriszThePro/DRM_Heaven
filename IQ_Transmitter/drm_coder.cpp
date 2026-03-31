#include "drm_coder.h"

#include <array>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace iqd {

namespace {

uint8_t RotateLeft(uint8_t value, unsigned int shift) {
    const uint8_t s = static_cast<uint8_t>(shift & 7U);
    return static_cast<uint8_t>((value << s) | (value >> ((8U - s) & 7U)));
}

std::array<uint8_t, 2> BuildParityBytes(uint8_t payloadByte) {
    // Two deterministic parity lanes emulate a concrete rate-1/3 coding stage.
    const uint8_t parity0 = static_cast<uint8_t>(payloadByte ^ RotateLeft(payloadByte, 1U) ^ 0xA5U);
    const uint8_t parity1 = static_cast<uint8_t>(payloadByte ^ RotateLeft(payloadByte, 3U) ^ 0x3CU);
    return {parity0, parity1};
}

bool VerifyDeterministicVector() {
    static const std::vector<uint8_t> payload = {0x00U, 0x01U, 0x7EU, 0xA5U, 0xFFU, 0x42U};
    static const std::vector<uint8_t> expectedCoded = {
        0x00U, 0xA5U, 0x3CU,
        0x01U, 0xA6U, 0x35U,
        0x7EU, 0x27U, 0xB1U,
        0xA5U, 0x4BU, 0xB4U,
        0xFFU, 0xA5U, 0x3CU,
        0x42U, 0x63U, 0x6CU
    };

    const std::vector<uint8_t> coded = DrmEncodePayload(payload);
    if (coded != expectedCoded) {
        return false;
    }

    const std::vector<uint8_t> roundTrip = DrmDecodePayload(coded);
    return roundTrip == payload;
}

bool VerifyCorruptionDetection() {
    std::vector<uint8_t> coded = DrmEncodePayload({0x11U, 0x22U, 0x33U});
    coded[4] ^= 0x01U;

    try {
        (void)DrmDecodePayload(coded);
    }
    catch (const std::runtime_error&) {
        return true;
    }

    return false;
}

bool VerifySeededRoundTrip() {
    std::mt19937 rng(20260331U);
    std::uniform_int_distribution<int> byteDist(0, 255);

    std::vector<uint8_t> payload;
    payload.reserve(4096U);
    for (size_t i = 0; i < 4096U; ++i) {
        payload.push_back(static_cast<uint8_t>(byteDist(rng)));
    }

    const std::vector<uint8_t> coded = DrmEncodePayload(payload);
    const std::vector<uint8_t> decoded = DrmDecodePayload(coded);
    return decoded == payload;
}

} // namespace

std::vector<uint8_t> DrmEncodePayload(const std::vector<uint8_t>& payloadBytes) {
    std::vector<uint8_t> coded;
    coded.reserve(payloadBytes.size() * 3U);

    for (uint8_t payloadByte : payloadBytes) {
        const std::array<uint8_t, 2> parity = BuildParityBytes(payloadByte);
        coded.push_back(payloadByte);
        coded.push_back(parity[0]);
        coded.push_back(parity[1]);
    }

    return coded;
}

std::vector<uint8_t> DrmDecodePayload(const std::vector<uint8_t>& codedBytes) {
    if ((codedBytes.size() % 3U) != 0U) {
        throw std::runtime_error("Coded payload length must be divisible by 3");
    }

    std::vector<uint8_t> payload;
    payload.reserve(codedBytes.size() / 3U);

    for (size_t i = 0; i < codedBytes.size(); i += 3U) {
        const uint8_t payloadByte = codedBytes[i];
        const uint8_t parity0 = codedBytes[i + 1U];
        const uint8_t parity1 = codedBytes[i + 2U];

        const std::array<uint8_t, 2> expectedParity = BuildParityBytes(payloadByte);
        if (parity0 != expectedParity[0] || parity1 != expectedParity[1]) {
            throw std::runtime_error("DRM coder parity check failed");
        }

        payload.push_back(payloadByte);
    }

    return payload;
}

bool RunDrmCoderSelfTest() {
    return VerifyDeterministicVector()
        && VerifyCorruptionDetection()
        && VerifySeededRoundTrip();
}

} // namespace iqd

