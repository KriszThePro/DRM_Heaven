#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cli.h"
#include "drm_coder.h"
#include "drm_interleaver.h"
#include "drm_logical_framer.h"
#include "drm_ofdm_modulator.h"
#include "drm_profiles.h"
#include "drm_tx_pipeline.h"
#include "file_io.h"
#include "fl2k_tx.h"
#include "iq_meta.h"
#include "m6_validation.h"
#include "m7_compliance.h"
#include "qpsk_decoder.h"

namespace {

std::string BuildReport(const iqd::Options& options, const iqd::DecodeResult& result, const iqd::IqMeta& meta) {
    std::ostringstream oss;
    oss
        << "input_file=" << options.inputPath << "\n"
        << "input_format=" << (options.inputS16 ? "s16" : "f32") << "\n"
        << "complex_samples=" << result.stats.complexSamples << "\n"
        << "i_mean=" << result.stats.iMean << "\n"
        << "q_mean=" << result.stats.qMean << "\n"
        << "i_rms=" << result.stats.iRms << "\n"
        << "q_rms=" << result.stats.qRms << "\n"
        << "mean_nearest_qpsk_distance=" << result.stats.meanNearestQpskDistance << "\n"
        << "recovered_bits=" << result.stats.recoveredBits << "\n"
        << "recovered_bytes=" << result.stats.recoveredBytes << "\n";
    if (meta.found) {
        oss
            << "meta_file=" << meta.path << "\n"
            << "meta_contract=" << meta.contract << "\n"
            << "meta_producer=" << meta.producer << "\n"
            << "meta_format=" << meta.format << "\n"
            << "meta_sample_rate_hz=" << meta.sampleRateHz << "\n"
            << "meta_scalar_samples=" << meta.scalarSamples << "\n"
            << "meta_complex_samples=" << meta.complexSamples << "\n";
    }
    return oss.str();
}

void ApplyAndValidateMeta(iqd::Options& options, const iqd::IqMeta& meta) {
    if (!meta.found) {
        return;
    }

    if (!options.formatExplicit && !meta.format.empty()) {
        if (meta.format == "f32") {
            options.inputS16 = false;
        } else if (meta.format == "s16") {
            options.inputS16 = true;
        } else {
            throw std::runtime_error("Unsupported format in iqmeta: " + meta.format);
        }
    }

    if (!meta.format.empty()) {
        const std::string actual = options.inputS16 ? "s16" : "f32";
        if (actual != meta.format) {
            throw std::runtime_error("IQ format mismatch with metadata (selected=" + actual + ", meta=" + meta.format + ")");
        }
    }
}

void ValidateSampleCountWithMeta(const std::vector<float>& iq, const iqd::IqMeta& meta) {
    if (!meta.found || meta.scalarSamples == 0U) {
        return;
    }
    if (iq.size() != meta.scalarSamples) {
        throw std::runtime_error("IQ scalar sample count mismatch with metadata");
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        iqd::Options options = iqd::ParseArgs(argc, argv);

        if (options.selfTest || options.selfTestM2 || options.selfTestM3 || options.selfTestM4 || options.selfTestM5 || options.selfTestM6 || options.selfTestM7) {
            if (options.selfTest) {
                const bool ok = iqd::RunSelfTest();
                if (!ok) {
                    throw std::runtime_error("Self-test failed");
                }
                std::cout << "Self-test: PASS\n";
            }

            if (options.selfTestM2) {
                const bool coderOk = iqd::RunDrmCoderSelfTest();
                const bool interleaverOk = iqd::RunDrmInterleaverSelfTest();
                if (!coderOk || !interleaverOk) {
                    throw std::runtime_error("M2 self-test failed");
                }
                std::cout << "Self-test-M2: PASS\n";
            }

            if (options.selfTestM3) {
                const bool framerOk = iqd::RunDrmLogicalFramerSelfTest();
                if (!framerOk) {
                    throw std::runtime_error("M3 self-test failed");
                }
                std::cout << "Self-test-M3: PASS\n";
            }

            if (options.selfTestM4) {
                const bool ofdmOk = iqd::RunDrmOfdmSelfTest();
                if (!ofdmOk) {
                    throw std::runtime_error("M4 self-test failed");
                }
                std::cout << "Self-test-M4: PASS\n";
            }

            if (options.selfTestM5) {
                const bool m5Ok = iqd::RunTxConditioningSelfTest();
                if (!m5Ok) {
                    throw std::runtime_error("M5 self-test failed");
                }
                std::cout << "Self-test-M5: PASS\n";
            }

            if (options.selfTestM6) {
                const bool m6Ok = iqd::RunM6SelfTest();
                if (!m6Ok) {
                    throw std::runtime_error("M6 self-test failed");
                }
                std::cout << "Self-test-M6: PASS\n";
            }

            if (options.selfTestM7) {
                const bool m7Ok = iqd::RunM7SelfTest();
                if (!m7Ok) {
                    throw std::runtime_error("M7 self-test failed");
                }
                std::cout << "Self-test-M7: PASS\n";
            }

            return 0;
        }

        iqd::IqMeta meta;
        if (!options.ignoreMeta) {
            (void)iqd::TryLoadIqMeta(options.inputPath, options.metaPath, meta);
            ApplyAndValidateMeta(options, meta);
        }

        const std::vector<float> iq = iqd::ReadInterleavedIqAsFloat(options.inputPath, options.inputS16);
        if (!options.ignoreMeta) {
            ValidateSampleCountWithMeta(iq, meta);
        }

        if (options.txFl2k) {
            if (!options.otaLabAuthorized) {
                throw std::runtime_error("FL2K OTA path is lab-only. Add --ota-lab-authorized after confirming legal authorization.");
            }

            iqd::TxConditioningConfig txCfg;
            txCfg.scale = options.txScale;
            txCfg.headroomDb = options.txHeadroomDb;
            txCfg.clip = options.txClip;
            txCfg.shape = options.txShape;

            iqd::TxConditioningTelemetry telem;
            const std::vector<uint8_t> fl2kBytes = iqd::ConvertIqToFl2kU8(iq, txCfg, &telem);
            iqd::StreamBytesToCommandStdin(options.fl2kCommand, fl2kBytes);

            std::cout
                << "input_file=" << options.inputPath << "\n"
                << "input_format=" << (options.inputS16 ? "s16" : "f32") << "\n"
                << "tx_mode=fl2k\n"
                << "fl2k_command=" << options.fl2kCommand << "\n"
                << "tx_region=" << options.txRegion << "\n"
                << "ota_lab_authorized=" << (options.otaLabAuthorized ? 1 : 0) << "\n"
                << "tx_scale=" << options.txScale << "\n"
                << "tx_headroom_db=" << options.txHeadroomDb << "\n"
                << "tx_clip=" << (options.txClip ? 1 : 0) << "\n"
                << "tx_shape=" << (options.txShape ? 1 : 0) << "\n"
                << "scalar_samples_streamed=" << fl2kBytes.size() << "\n"
                << "complex_samples_streamed=" << (fl2kBytes.size() / 2U) << "\n"
                << "tx_overrun_samples=" << telem.overrunSamples << "\n"
                << "tx_clipped_samples=" << telem.clippedSamples << "\n"
                << "tx_underrun_events=" << telem.underrunEvents << "\n"
                << "tx_peak_before=" << telem.peakBefore << "\n"
                << "tx_peak_after=" << telem.peakAfter << "\n";
            if (meta.found) {
                std::cout << "meta_file=" << meta.path << "\n";
            }
            return 0;
        }

        if (options.txDrm) {
            const iqd::DecodeResult decoded = iqd::DecodeIqQpskStream(iq);
            iqd::DrmPipelineConfig cfg;
            cfg.dumpStages = options.dumpStages;
            cfg.dumpDir = options.dumpDir;
            if (!iqd::ApplyDrmProfile(options.m6Profile, &cfg)) {
                throw std::runtime_error("Unknown M6 profile: " + options.m6Profile);
            }
            cfg.txScale = options.txScale;
            cfg.txHeadroomDb = options.txHeadroomDb;
            cfg.txClip = options.txClip;
            cfg.txShape = options.txShape;

            const iqd::DrmPipelineResult tx = iqd::RunDrmTxSkeleton(decoded.bytes, cfg);

            const iqd::M6KpiReport m6 = iqd::BuildM6KpiReport(decoded,
                                                              tx,
                                                              options.m6Profile,
                                                              options.runId,
                                                              options.labTag);
            std::string m6Reason;
            const bool m6Ok = iqd::ValidateM6KpiReport(m6, &m6Reason);
            const std::string m6Text = iqd::SerializeM6KpiReport(m6);
            if (!options.kpiOutPath.empty()) {
                iqd::WriteTextFile(options.kpiOutPath, m6Text);
            }

            const iqd::M7ComplianceReport m7 = iqd::BuildM7ComplianceReport(tx.ofdmIqF32Interleaved,
                                                                             options.txRegion,
                                                                             false);
            std::string m7Reason;
            const bool m7Ok = iqd::ValidateM7Compliance(m7, &m7Reason);

            std::cout
                << "input_file=" << options.inputPath << "\n"
                << "tx_mode=drm_m7\n"
                << "tx_region=" << options.txRegion << "\n"
                << "m6_profile=" << options.m6Profile << "\n"
                << "run_id=" << options.runId << "\n"
                << "lab_tag=" << options.labTag << "\n"
                << "payload_bytes=" << decoded.bytes.size() << "\n"
                << "coded_bytes=" << tx.codedBytes.size() << "\n"
                << "interleaved_bytes=" << tx.interleavedBytes.size() << "\n"
                << "logical_frames=" << tx.logicalFrameCount << "\n"
                << "ofdm_symbols=" << tx.ofdmSymbolCount << "\n"
                << "tx_scale=" << options.txScale << "\n"
                << "tx_headroom_db=" << options.txHeadroomDb << "\n"
                << "tx_clip=" << (options.txClip ? 1 : 0) << "\n"
                << "tx_shape=" << (options.txShape ? 1 : 0) << "\n"
                << "tx_overrun_samples=" << tx.txOverrunSamples << "\n"
                << "tx_clipped_samples=" << tx.txClippedSamples << "\n"
                << "tx_underrun_events=" << tx.txUnderrunEvents << "\n"
                << "tx_peak_before=" << tx.txPeakBefore << "\n"
                << "tx_peak_after=" << tx.txPeakAfter << "\n"
                << "kpi_lock_proxy=" << (m6.lockProxy ? 1 : 0) << "\n"
                << "kpi_decode_proxy=" << (m6.decodeProxy ? 1 : 0) << "\n"
                << "kpi_validation=" << (m6Ok ? "pass" : "fail") << "\n"
                << "kpi_validation_reason=" << m6Reason << "\n"
                << "m7_occupied_bw_ratio=" << m7.occupiedBandwidthRatio << "\n"
                << "m7_spur_suppression_db=" << m7.spurSuppressionDb << "\n"
                << "m7_region_allowed=" << (m7.regionAllowed ? 1 : 0) << "\n"
                << "m7_spectral_pass=" << (m7.spectralPass ? 1 : 0) << "\n"
                << "m7_validation=" << (m7Ok ? "pass" : "fail") << "\n"
                << "m7_validation_reason=" << m7Reason << "\n"
                << "framed_bytes=" << tx.framedBytes.size() << "\n"
                << "ofdm_iq_scalar_samples=" << tx.ofdmIqF32Interleaved.size() << "\n"
                << "ofdm_iq_complex_samples=" << (tx.ofdmIqF32Interleaved.size() / 2U) << "\n";
            if (options.dumpStages) {
                std::cout << "dump_dir=" << (options.dumpDir.empty() ? "." : options.dumpDir) << "\n";
            }
            if (!options.kpiOutPath.empty()) {
                std::cout << "kpi_file=" << options.kpiOutPath << "\n";
            }
            return 0;
        }

        const iqd::DecodeResult decoded = iqd::DecodeIqQpskStream(iq);

        iqd::WriteAllBytes(options.outputPath, decoded.bytes);

        const std::string report = BuildReport(options, decoded, meta);
        if (options.writeReport) {
            iqd::WriteTextFile(options.reportPath, report);
        }

        std::cout << report;
        std::cout << "output_file=" << options.outputPath << "\n";
        if (options.writeReport) {
            std::cout << "report_file=" << options.reportPath << "\n";
        }

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        iqd::PrintUsage((argc > 0 && argv[0] != nullptr) ? argv[0] : "IQ_Transmitter");
        return 1;
    }
}
