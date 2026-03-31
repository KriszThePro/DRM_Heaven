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
        << "  " << exe << " <input.iq> <output.bin> [--format f32|s16] [--report <report.txt>] [--meta <file.iqmeta>] [--no-meta]\n"
        << "  " << exe << " <input.iq> --tx-fl2k --fl2k-cmd \"<command reading raw u8 IQ on stdin>\" [--format f32|s16] [--meta <file.iqmeta>] [--no-meta] [--tx-scale <v>] [--tx-headroom-db <db>] [--tx-clip] [--tx-shape] [--region <LAB|EU|US|HU>] --ota-lab-authorized\n"
        << "  " << exe << " <input.iq> --tx-drm [--format f32|s16] [--meta <file.iqmeta>] [--no-meta] [--dump-stages] [--dump-dir <path>] [--tx-scale <v>] [--tx-headroom-db <db>] [--tx-clip] [--tx-shape] [--m6-profile <name>] [--kpi-out <path>] [--run-id <id>] [--lab-tag <tag>] [--region <LAB|EU|US|HU>]\n"
        << "  " << exe << " --self-test\n\n"
        << "  " << exe << " --self-test-m2\n\n"
        << "  " << exe << " --self-test-m3\n\n"
        << "  " << exe << " --self-test-m4\n\n"
        << "  " << exe << " --self-test-m5\n\n"
        << "  " << exe << " --self-test-m6\n\n"
        << "  " << exe << " --self-test-m7\n\n"
        << "Notes:\n"
        << "  - Input IQ is interleaved I,Q samples produced by MDI_to_IQ.\n"
        << "  - Decode mode writes recovered payload bits packed to bytes (MSB-first).\n"
        << "  - FL2K mode converts IQ to interleaved unsigned 8-bit samples and streams them to an external command via stdin.\n";
}

Options ParseArgs(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error("Missing arguments");
    }

    Options opt{};
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
                opt.formatExplicit = true;
            } else if (value == "s16") {
                opt.inputS16 = true;
                opt.formatExplicit = true;
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
        } else if (arg == "--self-test-m2") {
            opt.selfTestM2 = true;
        } else if (arg == "--self-test-m3") {
            opt.selfTestM3 = true;
        } else if (arg == "--self-test-m4") {
            opt.selfTestM4 = true;
        } else if (arg == "--self-test-m5") {
            opt.selfTestM5 = true;
        } else if (arg == "--self-test-m6") {
            opt.selfTestM6 = true;
        } else if (arg == "--self-test-m7") {
            opt.selfTestM7 = true;
        } else if (arg == "--tx-fl2k") {
            opt.txFl2k = true;
        } else if (arg == "--tx-drm") {
            opt.txDrm = true;
        } else if (arg == "--tx-scale") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--tx-scale requires a value");
            }
            opt.txScale = std::stof(argv[++i]);
            if (opt.txScale <= 0.0F) {
                throw std::runtime_error("--tx-scale must be > 0");
            }
        } else if (arg == "--tx-headroom-db") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--tx-headroom-db requires a value");
            }
            opt.txHeadroomDb = std::stof(argv[++i]);
            if (opt.txHeadroomDb < 0.0F) {
                throw std::runtime_error("--tx-headroom-db must be >= 0");
            }
        } else if (arg == "--tx-clip") {
            opt.txClip = true;
        } else if (arg == "--tx-shape") {
            opt.txShape = true;
        } else if (arg == "--m6-profile") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--m6-profile requires a value");
            }
            opt.m6Profile = argv[++i];
        } else if (arg == "--kpi-out") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--kpi-out requires a path");
            }
            opt.kpiOutPath = argv[++i];
        } else if (arg == "--run-id") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--run-id requires a value");
            }
            opt.runId = argv[++i];
        } else if (arg == "--lab-tag") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--lab-tag requires a value");
            }
            opt.labTag = argv[++i];
        } else if (arg == "--region") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--region requires a value");
            }
            opt.txRegion = argv[++i];
        } else if (arg == "--ota-lab-authorized") {
            opt.otaLabAuthorized = true;
        } else if (arg == "--fl2k-cmd") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--fl2k-cmd requires a command string");
            }
            opt.fl2kCommand = argv[++i];
        } else if (arg == "--dump-stages") {
            opt.dumpStages = true;
        } else if (arg == "--dump-dir") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--dump-dir requires a path");
            }
            opt.dumpDir = argv[++i];
        } else if (arg == "--meta") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--meta requires a path");
            }
            opt.metaPath = argv[++i];
        } else if (arg == "--no-meta") {
            opt.ignoreMeta = true;
        } else if (arg == "-h" || arg == "--help" || arg == "-?") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("Unknown option: " + arg);
        } else {
            positional.push_back(arg);
        }
    }

    if (opt.selfTest || opt.selfTestM2 || opt.selfTestM3 || opt.selfTestM4 || opt.selfTestM5 || opt.selfTestM6 || opt.selfTestM7) {
        return opt;
    }

    if (opt.txDrm && opt.txFl2k) {
        throw std::runtime_error("Choose only one TX mode: --tx-drm or --tx-fl2k");
    }

    if (opt.txDrm) {
        if (positional.size() != 1U) {
            throw std::runtime_error("DRM TX mode expects: <input.iq> --tx-drm");
        }
        opt.inputPath = positional[0];
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

