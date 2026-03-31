#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iqd {

struct Options {
    std::string inputPath;
    std::string outputPath;
    std::string reportPath;
    bool writeReport = false;
    bool inputS16 = false;
    bool selfTest = false;
    bool txFl2k = false;
    std::string fl2kCommand;
};

struct DecodeStats {
    size_t complexSamples = 0;
    double iMean = 0.0;
    double qMean = 0.0;
    double iRms = 0.0;
    double qRms = 0.0;
    double meanNearestQpskDistance = 0.0;
    size_t recoveredBits = 0;
    size_t recoveredBytes = 0;
};

struct DecodeResult {
    std::vector<uint8_t> bits;
    std::vector<uint8_t> bytes;
    DecodeStats stats;
};

} // namespace iqd

