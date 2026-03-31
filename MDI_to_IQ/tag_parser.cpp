#include "tag_parser.h"

#include <stdexcept>
#include <string>

#include "bit_reader.h"

namespace mdi {

void ParseTagPayload(const std::vector<uint8_t>& payload, MdiFrame& frame) {
    BitReader reader(payload);

    // ETSI TS 102 821 TAG payload: repeated {name(32), len_bits(32), data(len_bits)}
    while (reader.RemainingBits() >= 64U) {
        std::string tagName;
        tagName.reserve(4);
        for (int i = 0; i < 4; ++i) {
            tagName.push_back(static_cast<char>(reader.ReadBits(8)));
        }

        const uint32_t lenBits = reader.ReadBits(32);
        if (reader.RemainingBits() < lenBits) {
            throw std::runtime_error("Malformed TAG payload: tag length exceeds packet payload");
        }

        if (tagName == "dlfc" && lenBits == 32) {
            frame.dlfc = reader.ReadBits(32);
            frame.hasDlfc = true;
            continue;
        }

        if (tagName == "robm" && lenBits == 8) {
            frame.robustnessMode = static_cast<uint8_t>(reader.ReadBits(8));
            frame.hasRobustnessMode = true;
            continue;
        }

        if (tagName == "fac_") {
            frame.facBits = reader.ReadBitVector(lenBits);
            continue;
        }

        if (tagName == "sdc_") {
            frame.sdcBits = reader.ReadBitVector(lenBits);
            continue;
        }

        if (tagName.size() == 4 && tagName[0] == 's' && tagName[1] == 't' && tagName[2] == 'r' && tagName[3] >= '0' && tagName[3] <= '3') {
            const int streamIndex = tagName[3] - '0';
            frame.streamBits[streamIndex] = reader.ReadBitVector(lenBits);
            continue;
        }

        // Skip unsupported tags while preserving parser alignment.
        for (uint32_t i = 0; i < lenBits; ++i) {
            (void)reader.ReadBits(1);
        }
    }
}

} // namespace mdi

