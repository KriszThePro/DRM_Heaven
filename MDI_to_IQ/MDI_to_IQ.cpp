#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "af_decoder.h"
#include "cli.h"
#include "file_io.h"
#include "iq_meta.h"
#include "iq_processing.h"
#include "sample_mdi.h"

int main(int argc, char* argv[]) {
    try {
        const mdi::Options options = mdi::ParseArgs(argc, argv);

        if (options.generateSample) {
            mdi::GenerateSampleMdi(options.sampleOutputPath, options.sampleFrames);
            std::cout << "Generated sample MDI file: " << options.sampleOutputPath
                      << " (frames=" << options.sampleFrames << ")\n";
            return 0;
        }

        const std::vector<uint8_t> mdiBytes = mdi::ReadAllBytes(options.inputPath);
        const mdi::DecodeResult decoded = mdi::DecodeMdiAfPackets(mdiBytes, options.strictCrc);

        if (decoded.frames.empty()) {
            throw std::runtime_error(
                "No decodable AF packets found. Input may be text-dumped/corrupted rather than raw binary MDI AF data.");
        }

        const std::vector<float> iq = mdi::ConvertFramesToIq(decoded.frames);
        if (options.writeS16) {
            mdi::WriteIqS16(options.outputPath, iq);
        } else {
            mdi::WriteIqF32(options.outputPath, iq);
        }

        mdi::WriteIqMetaFile(options.outputPath,
                             options.writeS16,
                             options.sampleRate,
                             iq.size(),
                             decoded);

        std::cout
            << "Decoded AF packets: " << decoded.frames.size() << "\n"
            << "CRC warnings: " << decoded.crcWarnings << "\n"
            << "Resync warnings: " << decoded.resyncWarnings << "\n"
            << "Truncated tail warnings: " << decoded.truncatedTailWarnings << "\n"
            << "Sample rate metadata: " << options.sampleRate << " Hz\n"
            << "Wrote IQ samples (I,Q interleaved): " << (iq.size() / 2ULL) << "\n"
            << "Output file: " << options.outputPath << "\n"
            << "Metadata file: " << (options.outputPath + ".iqmeta") << "\n";

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        mdi::PrintUsage((argc > 0 && argv[0] != nullptr) ? argv[0] : "MDI_to_IQ");
        return 1;
    }
}
