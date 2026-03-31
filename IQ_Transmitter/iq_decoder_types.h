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
    bool formatExplicit = false;
    bool selfTest = false;
    bool selfTestM2 = false;
    bool selfTestM3 = false;
    bool selfTestM4 = false;
    bool selfTestM5 = false;
    bool selfTestM6 = false;
    bool selfTestM7 = false;
    bool txFl2k = false;
    std::string fl2kCommand;
    std::string metaPath;
    bool ignoreMeta = false;
    bool txDrm = false;
    bool dumpStages = false;
    std::string dumpDir;
    float txScale = 1.0F;
    float txHeadroomDb = 0.0F;
    bool txClip = false;
    bool txShape = false;
    std::string m6Profile = "balanced";
    std::string kpiOutPath;
    std::string runId = "run-default";
    std::string labTag = "local";
    std::string txRegion = "LAB";
    bool otaLabAuthorized = false;
};

struct IqMeta {
    bool found = false;
    std::string path;
    std::string contract;
    std::string producer;
    std::string format;
    int sampleRateHz = 0;
    size_t scalarSamples = 0;
    size_t complexSamples = 0;
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

