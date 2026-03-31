#include "sample_mdi.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "crc16.h"
#include "endian_utils.h"
#include "file_io.h"

namespace mdi {

namespace {

void AppendTag(std::vector<uint8_t>& payload, const std::string& name, const std::vector<uint8_t>& dataBits) {
    if (name.size() != 4) {
        throw std::runtime_error("Tag name must be 4 characters");
    }

    payload.insert(payload.end(), name.begin(), name.end());
    AppendBe32(payload, static_cast<uint32_t>(dataBits.size()));

    size_t bitIndex = 0;
    while (bitIndex < dataBits.size()) {
        uint8_t out = 0;
        for (int i = 0; i < 8; ++i) {
            out <<= 1U;
            if (bitIndex < dataBits.size()) {
                out |= (dataBits[bitIndex] & 0x01U);
                ++bitIndex;
            }
        }
        payload.push_back(out);
    }
}

std::vector<uint8_t> U32ToBits(uint32_t value) {
    std::vector<uint8_t> bits;
    bits.reserve(32);
    for (int i = 31; i >= 0; --i) {
        bits.push_back(static_cast<uint8_t>((value >> i) & 0x01U));
    }
    return bits;
}

std::vector<uint8_t> U8ToBits(uint8_t value) {
    std::vector<uint8_t> bits;
    bits.reserve(8);
    for (int i = 7; i >= 0; --i) {
        bits.push_back(static_cast<uint8_t>((value >> i) & 0x01U));
    }
    return bits;
}

std::vector<uint8_t> DeterministicBits(size_t count, uint32_t seed) {
    std::vector<uint8_t> bits;
    bits.reserve(count);
    uint32_t x = seed;
    for (size_t i = 0; i < count; ++i) {
        x = 1664525U * x + 1013904223U;
        bits.push_back(static_cast<uint8_t>((x >> 31U) & 0x01U));
    }
    return bits;
}

std::vector<uint8_t> BuildSampleAfPacket(uint16_t seq, uint32_t dlfc) {
    std::vector<uint8_t> payload;
    AppendTag(payload, "dlfc", U32ToBits(dlfc));
    AppendTag(payload, "robm", U8ToBits(0U));
    AppendTag(payload, "fac_", DeterministicBits(72, 0x1000U + seq));

    // ETSI-style sdc_ payload contains 4 RFU bits plus SDC bits; here we provide 40 bits total.
    AppendTag(payload, "sdc_", DeterministicBits(40, 0x2000U + seq));
    AppendTag(payload, "str0", DeterministicBits(2048, 0x3000U + seq));

    std::vector<uint8_t> packet;
    packet.push_back('A');
    packet.push_back('F');
    AppendBe32(packet, static_cast<uint32_t>(payload.size()));
    AppendBe16(packet, seq);

    const uint8_t crcFlag = 1U;
    const uint8_t afMajor = 1U;
    const uint8_t afMinor = 0U;
    const uint8_t ar = static_cast<uint8_t>((crcFlag << 7U) | ((afMajor & 0x07U) << 4U) | (afMinor & 0x0FU));
    packet.push_back(ar);
    packet.push_back(static_cast<uint8_t>('T'));

    packet.insert(packet.end(), payload.begin(), payload.end());
    const uint16_t crc = ComputeDreamCrc16(packet.data(), packet.size());
    AppendBe16(packet, crc);
    return packet;
}

} // namespace

void GenerateSampleMdi(const std::string& path, int frames) {
    if (frames <= 0) {
        throw std::runtime_error("--frames must be > 0");
    }

    std::vector<uint8_t> out;
    for (int i = 0; i < frames; ++i) {
        const auto pkt = BuildSampleAfPacket(static_cast<uint16_t>(i), static_cast<uint32_t>(1000 + i));
        out.insert(out.end(), pkt.begin(), pkt.end());
    }

    WriteAllBytes(path, out);
}

} // namespace mdi

