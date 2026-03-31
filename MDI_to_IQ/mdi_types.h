#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mdi {

struct Options {
    std::string inputPath;
    std::string outputPath;
    int sampleRate = 48000;
    bool strictCrc = false;
    bool writeS16 = false;
    bool generateSample = false;
    std::string sampleOutputPath;
    int sampleFrames = 4;
};

struct MdiFrame {
    uint16_t sequence = 0;
    uint8_t afMajor = 0;
    uint8_t afMinor = 0;
    uint32_t dlfc = 0;
    bool hasDlfc = false;
    uint8_t robustnessMode = 0;
    bool hasRobustnessMode = false;
    std::vector<uint8_t> facBits;
    std::vector<uint8_t> sdcBits;
    std::array<std::vector<uint8_t>, 4> streamBits;
};

struct DecodeResult {
    std::vector<MdiFrame> frames;
    int crcWarnings = 0;
    int truncatedTailWarnings = 0;
    int resyncWarnings = 0;
};

} // namespace mdi

