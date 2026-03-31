#include "fl2k_tx.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace iqd {

std::vector<uint8_t> ConvertIqToFl2kU8(const std::vector<float>& interleavedIq) {
    if (interleavedIq.size() % 2U != 0U) {
        throw std::runtime_error("IQ sample buffer must contain an even number of scalar values");
    }

    std::vector<uint8_t> out;
    out.reserve(interleavedIq.size());

    for (float v : interleavedIq) {
        const float clamped = std::max(-1.0F, std::min(1.0F, v));
        const float scaled = clamped * 127.0F + 127.5F;
        out.push_back(static_cast<uint8_t>(scaled));
    }

    return out;
}

void StreamBytesToCommandStdin(const std::string& command, const std::vector<uint8_t>& bytes) {
    FILE* pipe = _popen(command.c_str(), "wb");
    if (pipe == nullptr) {
        throw std::runtime_error("Failed to start external command for FL2K streaming");
    }

    const size_t written = fwrite(bytes.data(), 1U, bytes.size(), pipe);
    if (written != bytes.size()) {
        _pclose(pipe);
        throw std::runtime_error("Failed while writing FL2K byte stream to command stdin");
    }

    const int rc = _pclose(pipe);
    if (rc != 0) {
        throw std::runtime_error("External FL2K command exited with non-zero status");
    }
}

} // namespace iqd

