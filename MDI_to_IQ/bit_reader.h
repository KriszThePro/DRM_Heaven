#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mdi {

class BitReader {
public:
    explicit BitReader(const std::vector<uint8_t>& bytes) : bytes_(bytes) {}

    size_t RemainingBits() const {
        return bytes_.size() * 8ULL - bitPos_;
    }

    uint32_t ReadBits(size_t count) {
        if (count > 32 || count > RemainingBits()) {
            throw std::runtime_error("BitReader out-of-range read");
        }

        uint32_t value = 0;
        for (size_t i = 0; i < count; ++i) {
            const size_t absoluteBit = bitPos_ + i;
            const uint8_t byte = bytes_[absoluteBit / 8];
            const uint8_t bit = static_cast<uint8_t>((byte >> (7 - (absoluteBit % 8))) & 0x01U);
            value = static_cast<uint32_t>((value << 1) | bit);
        }

        bitPos_ += count;
        return value;
    }

    std::vector<uint8_t> ReadBitVector(size_t count) {
        std::vector<uint8_t> out;
        out.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            out.push_back(static_cast<uint8_t>(ReadBits(1)));
        }
        return out;
    }

private:
    const std::vector<uint8_t>& bytes_;
    size_t bitPos_ = 0;
};

} // namespace mdi

