#include "cli.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mdi {

void PrintUsage(const char* exe) {
    std::cout
        << "Usage:\n"
        << "  " << exe << " <input.mdi> <output.iq> [--format f32|s16] [--sample-rate 48000] [--strict-crc]\n"
        << "  " << exe << " --generate-sample <sample.mdi> [--frames 4]\n\n"
        << "Notes:\n"
        << "  - AF/TAG parsing follows ETSI TS 102 821 framing conventions.\n"
        << "  - Output is complex baseband IQ from QPSK mapping of decoded TAG bits.\n";
}

Options ParseArgs(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error("Missing arguments");
    }

    Options opt;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--strict-crc") {
            opt.strictCrc = true;
        } else if (arg == "--format") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--format requires a value");
            }
            const std::string value = argv[++i];
            if (value == "f32") {
                opt.writeS16 = false;
            } else if (value == "s16") {
                opt.writeS16 = true;
            } else {
                throw std::runtime_error("Unsupported format. Use f32 or s16");
            }
        } else if (arg == "--sample-rate") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--sample-rate requires a value");
            }
            opt.sampleRate = std::stoi(argv[++i]);
            if (opt.sampleRate <= 0) {
                throw std::runtime_error("sample-rate must be > 0");
            }
        } else if (arg == "--generate-sample") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--generate-sample requires an output file path");
            }
            opt.generateSample = true;
            opt.sampleOutputPath = argv[++i];
        } else if (arg == "--frames") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--frames requires a value");
            }
            opt.sampleFrames = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help" || arg == "-?") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("Unknown option: " + arg);
        } else {
            positional.push_back(arg);
        }
    }

    if (opt.generateSample) {
        return opt;
    }

    if (positional.size() != 2U) {
        throw std::runtime_error("Expected <input.mdi> <output.iq>");
    }

    opt.inputPath = positional[0];
    opt.outputPath = positional[1];
    return opt;
}

} // namespace mdi

