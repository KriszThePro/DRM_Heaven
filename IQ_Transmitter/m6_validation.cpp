#include "m6_validation.h"

#include <sstream>
#include <string>

namespace iqd {

M6KpiReport BuildM6KpiReport(const DecodeResult& decoded,
                             const DrmPipelineResult& tx,
                             const std::string& profile,
                             const std::string& runId,
                             const std::string& labTag) {
    M6KpiReport report;
    report.runId = runId;
    report.labTag = labTag;
    report.profile = profile;
    report.payloadBytes = decoded.bytes.size();
    report.logicalFrames = tx.logicalFrameCount;
    report.ofdmSymbols = tx.ofdmSymbolCount;
    report.meanNearestQpskDistance = decoded.stats.meanNearestQpskDistance;
    report.txPeakAfter = tx.txPeakAfter;
    report.txOverrunSamples = tx.txOverrunSamples;
    report.txUnderrunEvents = tx.txUnderrunEvents;

    report.lockProxy = (decoded.stats.meanNearestQpskDistance < 1.0)
        && (tx.txOverrunSamples == 0U)
        && (tx.txUnderrunEvents == 0U)
        && (tx.ofdmSymbolCount > 0U);

    report.decodeProxy = (decoded.stats.recoveredBytes > 0U)
        && (decoded.stats.recoveredBits >= decoded.stats.recoveredBytes * 8U)
        && (tx.logicalFrameCount > 0U);

    return report;
}

bool ValidateM6KpiReport(const M6KpiReport& report, std::string* reason) {
    if (!report.lockProxy) {
        if (reason != nullptr) {
            *reason = "lock_proxy=false";
        }
        return false;
    }

    if (!report.decodeProxy) {
        if (reason != nullptr) {
            *reason = "decode_proxy=false";
        }
        return false;
    }

    if (report.txPeakAfter > 1.0 + 1.0e-3) {
        if (reason != nullptr) {
            *reason = "tx_peak_after out of range";
        }
        return false;
    }

    if (reason != nullptr) {
        *reason = "ok";
    }
    return true;
}

std::string SerializeM6KpiReport(const M6KpiReport& report) {
    std::ostringstream oss;
    oss
        << "run_id=" << report.runId << "\n"
        << "lab_tag=" << report.labTag << "\n"
        << "profile=" << report.profile << "\n"
        << "payload_bytes=" << report.payloadBytes << "\n"
        << "logical_frames=" << report.logicalFrames << "\n"
        << "ofdm_symbols=" << report.ofdmSymbols << "\n"
        << "mean_nearest_qpsk_distance=" << report.meanNearestQpskDistance << "\n"
        << "tx_peak_after=" << report.txPeakAfter << "\n"
        << "tx_overrun_samples=" << report.txOverrunSamples << "\n"
        << "tx_underrun_events=" << report.txUnderrunEvents << "\n"
        << "lock_proxy=" << (report.lockProxy ? 1 : 0) << "\n"
        << "decode_proxy=" << (report.decodeProxy ? 1 : 0) << "\n";
    return oss.str();
}

bool RunM6SelfTest() {
    DecodeResult decoded;
    decoded.bytes = {0x11U, 0x22U, 0x33U};
    decoded.stats.recoveredBits = decoded.bytes.size() * 8U;
    decoded.stats.recoveredBytes = decoded.bytes.size();
    decoded.stats.meanNearestQpskDistance = 0.12;

    DrmPipelineResult tx;
    tx.logicalFrameCount = 2U;
    tx.ofdmSymbolCount = 7U;
    tx.txPeakAfter = 0.81F;
    tx.txOverrunSamples = 0U;
    tx.txUnderrunEvents = 0U;

    const M6KpiReport report = BuildM6KpiReport(decoded, tx, "balanced", "run-selftest", "ci");
    std::string reason;
    if (!ValidateM6KpiReport(report, &reason)) {
        return false;
    }

    const std::string text = SerializeM6KpiReport(report);
    return text.find("lock_proxy=1") != std::string::npos
        && text.find("decode_proxy=1") != std::string::npos
        && text.find("profile=balanced") != std::string::npos;
}

} // namespace iqd


