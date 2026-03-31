#include "iq_meta.h"

#include <sstream>
#include <string>

#include "file_io.h"

namespace mdi {

void WriteIqMetaFile(const std::string& iqPath,
                     bool outputS16,
                     int sampleRateHz,
                     size_t scalarSamples,
                     const DecodeResult& decodeStats) {
    const std::string metaPath = iqPath + ".iqmeta";

    std::ostringstream oss;
    oss
        << "contract=MDI_IQ_META_V1\n"
        << "producer=MDI_to_IQ\n"
        << "iq_file=" << iqPath << "\n"
        << "format=" << (outputS16 ? "s16" : "f32") << "\n"
        << "sample_rate_hz=" << sampleRateHz << "\n"
        << "scalar_samples=" << scalarSamples << "\n"
        << "complex_samples=" << (scalarSamples / 2U) << "\n"
        << "decoded_af_packets=" << decodeStats.frames.size() << "\n"
        << "crc_warnings=" << decodeStats.crcWarnings << "\n"
        << "resync_warnings=" << decodeStats.resyncWarnings << "\n"
        << "truncated_tail_warnings=" << decodeStats.truncatedTailWarnings << "\n";

    WriteTextFile(metaPath, oss.str());
}

} // namespace mdi

