#include "drm_logical_framer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace iqd {

namespace {

uint8_t XorChecksum(const std::vector<uint8_t>& bytes, size_t count) {
    uint8_t x = 0U;
    for (size_t i = 0; i < count; ++i) {
        x ^= bytes[i];
    }
    return x;
}

std::vector<uint8_t> BuildFac(uint8_t serviceId,
                              uint16_t frameIndex,
                              uint16_t totalFrames,
                              uint16_t mscLen) {
    std::vector<uint8_t> fac;
    fac.reserve(12U);
    fac.push_back('F');
    fac.push_back('A');
    fac.push_back('C');
    fac.push_back('0');
    fac.push_back(serviceId);
    fac.push_back(static_cast<uint8_t>(frameIndex & 0xFFU));
    fac.push_back(static_cast<uint8_t>((frameIndex >> 8) & 0xFFU));
    fac.push_back(static_cast<uint8_t>(totalFrames & 0xFFU));
    fac.push_back(static_cast<uint8_t>((totalFrames >> 8) & 0xFFU));
    fac.push_back(static_cast<uint8_t>(mscLen & 0xFFU));
    fac.push_back(static_cast<uint8_t>((mscLen >> 8) & 0xFFU));
    fac.push_back(XorChecksum(fac, 11U));
    return fac;
}

std::vector<uint8_t> BuildSdc(uint8_t serviceId,
                              uint16_t frameIndex,
                              uint16_t totalFrames,
                              uint16_t mscLen) {
    std::vector<uint8_t> sdc;
    sdc.reserve(14U);
    sdc.push_back('S');
    sdc.push_back('D');
    sdc.push_back('C');
    sdc.push_back('0');
    sdc.push_back(serviceId);
    sdc.push_back(static_cast<uint8_t>(frameIndex & 0xFFU));
    sdc.push_back(static_cast<uint8_t>((frameIndex >> 8) & 0xFFU));
    sdc.push_back(static_cast<uint8_t>(totalFrames & 0xFFU));
    sdc.push_back(static_cast<uint8_t>((totalFrames >> 8) & 0xFFU));
    sdc.push_back(0x11U);
    sdc.push_back(0x01U);
    sdc.push_back(static_cast<uint8_t>(mscLen & 0xFFU));
    sdc.push_back(static_cast<uint8_t>((mscLen >> 8) & 0xFFU));
    sdc.push_back(XorChecksum(sdc, 13U));
    return sdc;
}

uint16_t ReadLe16(const std::vector<uint8_t>& in, size_t offset) {
    return static_cast<uint16_t>(
        static_cast<uint16_t>(in[offset]) |
        (static_cast<uint16_t>(in[offset + 1U]) << 8U));
}

void ValidateFrame(const DrmLogicalFrame& frame, uint8_t expectedServiceId) {
    if (frame.facBytes.size() != 12U || frame.sdcBytes.size() != 14U) {
        throw std::runtime_error("Invalid FAC/SDC header size");
    }

    if (!(frame.facBytes[0] == 'F' && frame.facBytes[1] == 'A' && frame.facBytes[2] == 'C' && frame.facBytes[3] == '0')) {
        throw std::runtime_error("FAC signature mismatch");
    }
    if (!(frame.sdcBytes[0] == 'S' && frame.sdcBytes[1] == 'D' && frame.sdcBytes[2] == 'C' && frame.sdcBytes[3] == '0')) {
        throw std::runtime_error("SDC signature mismatch");
    }

    if (frame.facBytes[11] != XorChecksum(frame.facBytes, 11U)) {
        throw std::runtime_error("FAC checksum mismatch");
    }
    if (frame.sdcBytes[13] != XorChecksum(frame.sdcBytes, 13U)) {
        throw std::runtime_error("SDC checksum mismatch");
    }

    const uint8_t facService = frame.facBytes[4];
    const uint8_t sdcService = frame.sdcBytes[4];
    if (facService != expectedServiceId || sdcService != expectedServiceId || facService != sdcService) {
        throw std::runtime_error("Service mapping mismatch between FAC/SDC and pipeline config");
    }

    const uint16_t facIndex = ReadLe16(frame.facBytes, 5U);
    const uint16_t sdcIndex = ReadLe16(frame.sdcBytes, 5U);
    if (facIndex != frame.frameIndex || sdcIndex != frame.frameIndex || facIndex != sdcIndex) {
        throw std::runtime_error("Frame index mismatch between FAC/SDC");
    }

    const uint16_t facTotal = ReadLe16(frame.facBytes, 7U);
    const uint16_t sdcTotal = ReadLe16(frame.sdcBytes, 7U);
    if (facTotal != frame.totalFrames || sdcTotal != frame.totalFrames || facTotal != sdcTotal) {
        throw std::runtime_error("Total frame count mismatch between FAC/SDC");
    }

    const uint16_t facMscLen = ReadLe16(frame.facBytes, 9U);
    const uint16_t sdcMscLen = ReadLe16(frame.sdcBytes, 11U);
    const uint16_t actualMscLen = static_cast<uint16_t>(frame.mscBytes.size());
    if (facMscLen != actualMscLen || sdcMscLen != actualMscLen || facMscLen != sdcMscLen) {
        throw std::runtime_error("MSC length mismatch between FAC/SDC and payload");
    }
}

std::vector<uint8_t> PackOneFrame(const DrmLogicalFrame& frame) {
    std::vector<uint8_t> packed;
    packed.reserve(11U + frame.facBytes.size() + frame.sdcBytes.size() + frame.mscBytes.size());
    packed.push_back(0xF3U);
    packed.push_back(0x4DU);
    packed.push_back(static_cast<uint8_t>(frame.frameIndex & 0xFFU));
    packed.push_back(static_cast<uint8_t>((frame.frameIndex >> 8) & 0xFFU));
    packed.push_back(static_cast<uint8_t>(frame.totalFrames & 0xFFU));
    packed.push_back(static_cast<uint8_t>((frame.totalFrames >> 8) & 0xFFU));
    packed.push_back(frame.serviceId);
    packed.push_back(static_cast<uint8_t>(frame.facBytes.size()));
    packed.push_back(static_cast<uint8_t>(frame.sdcBytes.size()));
    packed.push_back(static_cast<uint8_t>(frame.mscBytes.size() & 0xFFU));
    packed.push_back(static_cast<uint8_t>((frame.mscBytes.size() >> 8U) & 0xFFU));
    packed.insert(packed.end(), frame.facBytes.begin(), frame.facBytes.end());
    packed.insert(packed.end(), frame.sdcBytes.begin(), frame.sdcBytes.end());
    packed.insert(packed.end(), frame.mscBytes.begin(), frame.mscBytes.end());
    return packed;
}

bool VerifyDeterministicVector() {
    std::vector<uint8_t> in(20U, 0U);
    for (size_t i = 0; i < in.size(); ++i) {
        in[i] = static_cast<uint8_t>(i);
    }

    const std::vector<DrmLogicalFrame> frames = AssembleDrmLogicalFrames(in, 7U, 8U);
    if (frames.size() != 3U) {
        return false;
    }

    static const std::vector<uint8_t> expectedFac0 = {
        'F','A','C','0', 7U, 0U, 0U, 3U, 0U, 8U, 0U, 0x78U
    };
    static const std::vector<uint8_t> expectedSdc0 = {
        'S','D','C','0', 7U, 0U, 0U, 3U, 0U, 0x11U, 0x01U, 8U, 0U, 0x78U
    };
    if (frames[0].facBytes != expectedFac0 || frames[0].sdcBytes != expectedSdc0) {
        return false;
    }

    std::vector<uint8_t> expectedMsc0 = {0U,1U,2U,3U,4U,5U,6U,7U};
    if (frames[0].mscBytes != expectedMsc0) {
        return false;
    }

    const std::vector<uint8_t> packed = PackDrmLogicalFrames(frames);
    if (packed.empty()) {
        return false;
    }

    return packed[0] == 0xF3U && packed[1] == 0x4DU;
}

bool VerifyLengthAccounting() {
    std::vector<uint8_t> in(257U, 0U);
    for (size_t i = 0; i < in.size(); ++i) {
        in[i] = static_cast<uint8_t>((i * 37U) & 0xFFU);
    }

    const std::vector<DrmLogicalFrame> frames = AssembleDrmLogicalFrames(in, 3U, 64U);
    std::vector<uint8_t> rebuilt;
    for (const DrmLogicalFrame& frame : frames) {
        rebuilt.insert(rebuilt.end(), frame.mscBytes.begin(), frame.mscBytes.end());
    }

    if (rebuilt != in) {
        return false;
    }

    const std::vector<uint8_t> packed = PackDrmLogicalFrames(frames);
    if (packed.size() <= in.size()) {
        return false;
    }

    return true;
}

} // namespace

std::vector<DrmLogicalFrame> AssembleDrmLogicalFrames(const std::vector<uint8_t>& interleavedBytes,
                                                      uint8_t serviceId,
                                                      size_t mscBytesPerFrame) {
    if (interleavedBytes.empty()) {
        throw std::runtime_error("DRM logical framer received empty interleaved payload");
    }
    if (mscBytesPerFrame == 0U || mscBytesPerFrame > 65535U) {
        throw std::runtime_error("Invalid mscBytesPerFrame");
    }

    const size_t totalFramesSizeT = (interleavedBytes.size() + mscBytesPerFrame - 1U) / mscBytesPerFrame;
    if (totalFramesSizeT == 0U || totalFramesSizeT > 65535U) {
        throw std::runtime_error("Unsupported logical frame count");
    }

    const uint16_t totalFrames = static_cast<uint16_t>(totalFramesSizeT);
    std::vector<DrmLogicalFrame> frames;
    frames.reserve(totalFramesSizeT);

    size_t totalMscBytes = 0U;
    for (size_t i = 0; i < totalFramesSizeT; ++i) {
        const size_t offset = i * mscBytesPerFrame;
        const size_t remain = interleavedBytes.size() - offset;
        const size_t mscLen = std::min(mscBytesPerFrame, remain);

        DrmLogicalFrame frame;
        frame.frameIndex = static_cast<uint16_t>(i);
        frame.totalFrames = totalFrames;
        frame.serviceId = serviceId;
        frame.mscBytes.insert(frame.mscBytes.end(),
                              interleavedBytes.begin() + static_cast<std::ptrdiff_t>(offset),
                              interleavedBytes.begin() + static_cast<std::ptrdiff_t>(offset + mscLen));

        frame.facBytes = BuildFac(serviceId, frame.frameIndex, frame.totalFrames, static_cast<uint16_t>(mscLen));
        frame.sdcBytes = BuildSdc(serviceId, frame.frameIndex, frame.totalFrames, static_cast<uint16_t>(mscLen));

        ValidateFrame(frame, serviceId);
        frame.packedBytes = PackOneFrame(frame);
        frames.push_back(frame);

        totalMscBytes += mscLen;
    }

    if (totalMscBytes != interleavedBytes.size()) {
        throw std::runtime_error("Logical frame assembly length mismatch");
    }

    for (size_t i = 0; i < frames.size(); ++i) {
        const DrmLogicalFrame& frame = frames[i];
        if (frame.frameIndex != static_cast<uint16_t>(i)) {
            throw std::runtime_error("Non-sequential logical frame index");
        }
        if (frame.totalFrames != totalFrames || frame.serviceId != serviceId) {
            throw std::runtime_error("Logical frame metadata mismatch");
        }
    }

    return frames;
}

std::vector<uint8_t> PackDrmLogicalFrames(const std::vector<DrmLogicalFrame>& frames) {
    std::vector<uint8_t> out;

    size_t total = 0U;
    for (const DrmLogicalFrame& frame : frames) {
        total += frame.packedBytes.size();
    }
    out.reserve(total);

    for (const DrmLogicalFrame& frame : frames) {
        out.insert(out.end(), frame.packedBytes.begin(), frame.packedBytes.end());
    }

    return out;
}

bool RunDrmLogicalFramerSelfTest() {
    return VerifyDeterministicVector() && VerifyLengthAccounting();
}

} // namespace iqd


