#pragma once

#include <string>
#include <vector>

namespace iqd {

struct M7ComplianceReport {
    std::string region;
    bool regionAllowed = false;
    bool otaAuthorized = false;
    double occupiedBandwidthRatio = 0.0;
    double spurSuppressionDb = 0.0;
    bool spectralPass = false;
};

M7ComplianceReport BuildM7ComplianceReport(const std::vector<float>& ofdmIqInterleaved,
                                           const std::string& region,
                                           bool otaAuthorized);

bool ValidateM7Compliance(const M7ComplianceReport& report, std::string* reason);
bool RunM7SelfTest();

} // namespace iqd

