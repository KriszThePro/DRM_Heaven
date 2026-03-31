#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cli.h"
#include "file_io.h"
#include "fl2k_tx.h"
#include "qpsk_decoder.h"

namespace {

std::string BuildReport(const iqd::Options& options, const iqd::DecodeResult& result) {
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
    return oss.str();
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const iqd::Options options = iqd::ParseArgs(argc, argv);

        if (options.selfTest) {
            const bool ok = iqd::RunSelfTest();
            if (!ok) {
                throw std::runtime_error("Self-test failed");
            }
            std::cout << "Self-test: PASS\n";
            return 0;
        }

        const std::vector<float> iq = iqd::ReadInterleavedIqAsFloat(options.inputPath, options.inputS16);

        if (options.txFl2k) {
            const std::vector<uint8_t> fl2kBytes = iqd::ConvertIqToFl2kU8(iq);
            iqd::StreamBytesToCommandStdin(options.fl2kCommand, fl2kBytes);

            std::cout
                << "input_file=" << options.inputPath << "\n"
                << "input_format=" << (options.inputS16 ? "s16" : "f32") << "\n"
                << "tx_mode=fl2k\n"
                << "fl2k_command=" << options.fl2kCommand << "\n"
                << "scalar_samples_streamed=" << fl2kBytes.size() << "\n"
                << "complex_samples_streamed=" << (fl2kBytes.size() / 2U) << "\n";
            return 0;
        }

        const iqd::DecodeResult decoded = iqd::DecodeIqQpskStream(iq);

        iqd::WriteAllBytes(options.outputPath, decoded.bytes);

        const std::string report = BuildReport(options, decoded);
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
