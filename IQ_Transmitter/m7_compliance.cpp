#include "m7_compliance.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace iqd {

namespace {

bool IsRegionAllowed(const std::string& region) {
    return region == "LAB" || region == "EU" || region == "US" || region == "HU";
}

std::vector<double> ComputeSpectrumPower(const std::vector<float>& iqInterleaved) {
    if (iqInterleaved.size() < 4U || (iqInterleaved.size() % 2U) != 0U) {
        throw std::runtime_error("M7 spectral check requires non-empty complex IQ stream");
    }

    const size_t nComplex = iqInterleaved.size() / 2U;
    const size_t n = std::min<size_t>(512U, nComplex);

    std::vector<std::complex<double>> x;
    x.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        x.emplace_back(static_cast<double>(iqInterleaved[2U * i]),
                       static_cast<double>(iqInterleaved[2U * i + 1U]));
    }

    const double twoPi = 6.283185307179586;
    std::vector<double> power(n, 0.0);

    for (size_t k = 0; k < n; ++k) {
        std::complex<double> sum(0.0, 0.0);
        for (size_t t = 0; t < n; ++t) {
            const double angle = -twoPi * static_cast<double>(k * t) / static_cast<double>(n);
            const std::complex<double> w(std::cos(angle), std::sin(angle));
            sum += x[t] * w;
        }
        power[k] = std::norm(sum);
    }

    return power;
}

void ComputeSpectralMetrics(const std::vector<double>& power,
                            double* occupiedRatio,
                            double* spurSuppressionDb,
                            bool* spectralPass) {
    const size_t n = power.size();
    const double totalPower = std::accumulate(power.begin(), power.end(), 0.0);
    if (n == 0U || totalPower <= 0.0) {
        *occupiedRatio = 1.0;
        *spurSuppressionDb = 0.0;
        *spectralPass = false;
        return;
    }

    std::vector<double> sorted = power;
    std::sort(sorted.begin(), sorted.end(), std::greater<double>());

    double acc = 0.0;
    size_t occBins = 0U;
    while (occBins < n && acc < 0.99 * totalPower) {
        acc += sorted[occBins];
        ++occBins;
    }

    *occupiedRatio = static_cast<double>(occBins) / static_cast<double>(n);

    const double peak = sorted[0];
    const double maxOut = (occBins < n) ? sorted[occBins] : 0.0;
    *spurSuppressionDb = 10.0 * std::log10((peak + 1.0e-15) / (maxOut + 1.0e-15));

    *spectralPass = (*occupiedRatio <= 0.98) && (*spurSuppressionDb >= 0.2);
}

} // namespace

M7ComplianceReport BuildM7ComplianceReport(const std::vector<float>& ofdmIqInterleaved,
                                           const std::string& region,
                                           bool otaAuthorized) {
    M7ComplianceReport report;
    report.region = region;
    report.regionAllowed = IsRegionAllowed(region);
    report.otaAuthorized = otaAuthorized;

    const std::vector<double> power = ComputeSpectrumPower(ofdmIqInterleaved);
    ComputeSpectralMetrics(power,
                           &report.occupiedBandwidthRatio,
                           &report.spurSuppressionDb,
                           &report.spectralPass);
    return report;
}

bool ValidateM7Compliance(const M7ComplianceReport& report, std::string* reason) {
    if (!report.regionAllowed) {
        if (reason != nullptr) {
            *reason = "region_not_allowed";
        }
        return false;
    }

    if (!report.spectralPass) {
        if (reason != nullptr) {
            *reason = "spectral_check_failed";
        }
        return false;
    }

    if (reason != nullptr) {
        *reason = "ok";
    }
    return true;
}

bool RunM7SelfTest() {
    std::vector<float> iq;
    iq.reserve(2048U);
    const double twoPi = 6.283185307179586;
    for (size_t n = 0U; n < 1024U; ++n) {
        const double a = twoPi * 0.08 * static_cast<double>(n);
        iq.push_back(static_cast<float>(0.6 * std::cos(a)));
        iq.push_back(static_cast<float>(0.6 * std::sin(a)));
    }

    const M7ComplianceReport ok = BuildM7ComplianceReport(iq, "LAB", true);
    std::string reason;
    if (!ValidateM7Compliance(ok, &reason)) {
        return false;
    }

    const M7ComplianceReport badRegion = BuildM7ComplianceReport(iq, "XX", true);
    if (ValidateM7Compliance(badRegion, &reason)) {
        return false;
    }

    return true;
}

} // namespace iqd


