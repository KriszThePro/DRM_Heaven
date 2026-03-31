#include "cli.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace iqd {

void PrintUsage(const char* exe) {
    std::cout
        << "Usage:\n"
        << "  " << exe << " <input.iq> <output.bin> [--format f32|s16] [--report <report.txt>]\n"
        << "  " << exe << " <input.iq> --tx-fl2k --fl2k-cmd \"<command reading raw u8 IQ on stdin>\" [--format f32|s16]\n"
        << "  " << exe << " --self-test\n\n"
        << "Notes:\n"
        << "  - Input IQ is interleaved I,Q samples produced by MDI_to_IQ.\n"
        << "  - Decode mode writes recovered payload bits packed to bytes (MSB-first).\n"
        << "  - FL2K mode converts IQ to interleaved unsigned 8-bit samples and streams them to an external command via stdin.\n";
}

Options ParseArgs(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error("Missing arguments");
    }

    Options opt;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--format") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--format requires a value");
            }
            const std::string value = argv[++i];
            if (value == "f32") {
                opt.inputS16 = false;
            } else if (value == "s16") {
                opt.inputS16 = true;
            } else {
                throw std::runtime_error("Unsupported format. Use f32 or s16");
            }
        } else if (arg == "--report") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--report requires a path");
            }
            opt.writeReport = true;
            opt.reportPath = argv[++i];
        } else if (arg == "--self-test") {
            opt.selfTest = true;
        } else if (arg == "--tx-fl2k") {
            opt.txFl2k = true;
        } else if (arg == "--fl2k-cmd") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--fl2k-cmd requires a command string");
            }
            opt.fl2kCommand = argv[++i];
        } else if (arg == "-h" || arg == "--help" || arg == "-?") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("Unknown option: " + arg);
        } else {
            positional.push_back(arg);
        }
    }

    if (opt.selfTest) {
        return opt;
    }

    if (opt.txFl2k) {
        if (positional.size() != 1U) {
            throw std::runtime_error("FL2K mode expects: <input.iq> --tx-fl2k --fl2k-cmd <command>");
        }
        if (opt.fl2kCommand.empty()) {
            throw std::runtime_error("FL2K mode requires --fl2k-cmd");
        }
        opt.inputPath = positional[0];
        return opt;
    }

    if (positional.size() != 2U) {
        throw std::runtime_error("Expected <input.iq> <output.bin>");
    }

    opt.inputPath = positional[0];
    opt.outputPath = positional[1];
    return opt;
}

} // namespace iqd

