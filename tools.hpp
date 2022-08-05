#pragma once
#include <fstream>
#include <vector>
#include <string>

namespace pasimulations {
namespace tools {

// This will be part of c++23
// From: https://stackoverflow.com/a/8357462/19477646
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

constexpr auto hash(const std::string_view data) noexcept {
    uint32_t hash = 5381;

    for (const auto letter : data)
        hash = ((hash << 5) + hash) + (unsigned char)letter;

    return hash;
}


[[nodiscard]][[maybe_unused]]static std::vector<uint32_t> readShader(const std::string shader_path) {
    std::ifstream fileStream(shader_path, std::ios::binary);
    std::vector<char> buffer;
    buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
    return { (uint32_t*)buffer.data(), (uint32_t*)(buffer.data() + buffer.size()) };
}


template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}


} // namespace tools
} // namespace pasimulations