#include "iq_meta.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace iqd {

namespace {

std::string Trim(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) {
        ++b;
    }
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) {
        --e;
    }
    return s.substr(b, e - b);
}

void ApplyKeyValue(const std::string& key, const std::string& value, IqMeta& meta) {
    if (key == "contract") {
        meta.contract = value;
    } else if (key == "producer") {
        meta.producer = value;
    } else if (key == "format") {
        meta.format = value;
    } else if (key == "sample_rate_hz") {
        meta.sampleRateHz = std::stoi(value);
    } else if (key == "scalar_samples") {
        meta.scalarSamples = static_cast<size_t>(std::stoull(value));
    } else if (key == "complex_samples") {
        meta.complexSamples = static_cast<size_t>(std::stoull(value));
    }
}

} // namespace

bool TryLoadIqMeta(const std::string& iqPath, const std::string& explicitMetaPath, IqMeta& meta) {
    const std::string metaPath = explicitMetaPath.empty() ? (iqPath + ".iqmeta") : explicitMetaPath;
    std::ifstream in(metaPath, std::ios::binary);
    if (!in) {
        meta = IqMeta();
        return false;
    }

    meta = IqMeta();
    meta.found = true;
    meta.path = metaPath;

    std::string line;
    while (std::getline(in, line)) {
        const std::string clean = Trim(line);
        if (clean.empty()) {
            continue;
        }
        const size_t pos = clean.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const std::string key = Trim(clean.substr(0, pos));
        const std::string value = Trim(clean.substr(pos + 1));
        ApplyKeyValue(key, value, meta);
    }

    if (meta.contract.empty()) {
        throw std::runtime_error("IQ metadata file missing contract field: " + meta.path);
    }
    if (meta.contract != "MDI_IQ_META_V1") {
        throw std::runtime_error("Unsupported IQ metadata contract: " + meta.contract);
    }

    return true;
}

} // namespace iqd

