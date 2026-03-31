#include "file_io.h"

#include <fstream>
#include <stdexcept>

namespace iqd {

std::vector<float> ReadInterleavedIqAsFloat(const std::string& path, bool inputS16) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open input IQ file: " + path);
    }

    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    in.seekg(0, std::ios::beg);

    if (size <= 0) {
        throw std::runtime_error("Input IQ file is empty: " + path);
    }

    std::vector<float> out;

    if (!inputS16) {
        if ((size % static_cast<std::streamoff>(sizeof(float))) != 0) {
            throw std::runtime_error("f32 IQ file size is not a multiple of 4 bytes");
        }

        const size_t count = static_cast<size_t>(size / static_cast<std::streamoff>(sizeof(float)));
        out.resize(count);
        in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(count * sizeof(float)));
    } else {
        if ((size % static_cast<std::streamoff>(sizeof(int16_t))) != 0) {
            throw std::runtime_error("s16 IQ file size is not a multiple of 2 bytes");
        }

        const size_t count = static_cast<size_t>(size / static_cast<std::streamoff>(sizeof(int16_t)));
        std::vector<int16_t> packed(count);
        in.read(reinterpret_cast<char*>(packed.data()), static_cast<std::streamsize>(count * sizeof(int16_t)));

        out.reserve(count);
        for (int16_t v : packed) {
            out.push_back(static_cast<float>(v) / 32767.0F);
        }
    }

    if (!in) {
        throw std::runtime_error("Failed while reading IQ file: " + path);
    }

    if (out.size() % 2U != 0U) {
        throw std::runtime_error("IQ scalar sample count is odd; expected interleaved I,Q pairs");
    }

    return out;
}

void WriteAllBytes(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open output file: " + path);
    }

    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        throw std::runtime_error("Failed while writing output file: " + path);
    }
}

void WriteTextFile(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open report file: " + path);
    }

    out << text;
    if (!out) {
        throw std::runtime_error("Failed while writing report file: " + path);
    }
}

} // namespace iqd

