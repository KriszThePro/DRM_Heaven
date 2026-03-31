#include "file_io.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace mdi {

std::vector<uint8_t> ReadAllBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open input file: " + path);
    }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
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

} // namespace mdi

