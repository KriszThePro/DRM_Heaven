#include "af_decoder.h"

#include <sstream>
#include <stdexcept>
#include <string>

#include "crc16.h"
#include "endian_utils.h"
#include "tag_parser.h"

namespace mdi {

DecodeResult DecodeMdiAfPackets(const std::vector<uint8_t>& fileBytes, bool strictCrc) {
    DecodeResult result;
    size_t offset = 0;

    while (offset < fileBytes.size()) {
        if (fileBytes.size() - offset < 12U) {
            // Some captures end with short non-AF tails; keep decoded frames.
            ++result.truncatedTailWarnings;
            break;
        }

        if (!(fileBytes[offset] == 'A' && fileBytes[offset + 1] == 'F')) {
            size_t next = std::string::npos;
            for (size_t i = offset + 1; i + 1 < fileBytes.size(); ++i) {
                if (fileBytes[i] == 'A' && fileBytes[i + 1] == 'F') {
                    next = i;
                    break;
                }
            }

            if (next == std::string::npos) {
                if (result.frames.empty()) {
                    std::ostringstream oss;
                    oss << "AF sync not found at byte offset " << offset;
                    throw std::runtime_error(oss.str());
                }
                ++result.truncatedTailWarnings;
                break;
            }

            ++result.resyncWarnings;
            offset = next;
            continue;
        }

        const uint32_t payloadLen = ReadBe32(&fileBytes[offset + 2]);
        const uint16_t seq = ReadBe16(&fileBytes[offset + 6]);
        const uint8_t ar = fileBytes[offset + 8];
        const uint8_t cf = static_cast<uint8_t>((ar >> 7U) & 0x01U);
        const uint8_t major = static_cast<uint8_t>((ar >> 4U) & 0x07U);
        const uint8_t minor = static_cast<uint8_t>(ar & 0x0FU);
        const uint8_t protocolType = fileBytes[offset + 9];

        if (protocolType != 'T') {
            std::ostringstream oss;
            oss << "Unsupported AF payload protocol type 0x" << std::hex << static_cast<int>(protocolType)
                << " (expected 'T' for TAG packets)";
            throw std::runtime_error(oss.str());
        }

        const size_t packetBytes = 10ULL + payloadLen + 2ULL;
        if (offset + packetBytes > fileBytes.size()) {
            ++result.truncatedTailWarnings;
            break;
        }

        const uint16_t expectedCrc = ReadBe16(&fileBytes[offset + 10 + payloadLen]);
        const uint16_t computedCrc = ComputeDreamCrc16(&fileBytes[offset], static_cast<size_t>(10U + payloadLen));
        const bool crcOk = (expectedCrc == computedCrc);
        if (cf != 0 && !crcOk) {
            ++result.crcWarnings;
            if (strictCrc) {
                std::ostringstream oss;
                oss << "CRC mismatch in AF packet at offset " << offset;
                throw std::runtime_error(oss.str());
            }
        }

        std::vector<uint8_t> payload(fileBytes.begin() + static_cast<std::ptrdiff_t>(offset + 10U),
                                     fileBytes.begin() + static_cast<std::ptrdiff_t>(offset + 10U + payloadLen));

        MdiFrame frame;
        frame.sequence = seq;
        frame.afMajor = major;
        frame.afMinor = minor;
        ParseTagPayload(payload, frame);
        result.frames.push_back(std::move(frame));

        offset += packetBytes;
    }

    return result;
}

} // namespace mdi

