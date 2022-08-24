#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <concepts>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define KOMPUTE_LOG_LEVEL 5
#include "../libs/kompute/single_include/kompute/Kompute.hpp"
#pragma GCC diagnostic pop
namespace pasimulations {
namespace tools {

// Cast enum class to underlying (integer) type
// This will be part of c++23
// From: https://stackoverflow.com/a/8357462/19477646
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

inline constexpr auto hash(const std::string_view data) noexcept {
    uint32_t hash = 5381;

    for (const auto letter : data)
        hash = ((hash << 5) + hash) + (unsigned char)letter;

    return hash;
}

[[nodiscard]] [[maybe_unused]] inline std::vector<uint32_t> readShader(const std::filesystem::path path_to_shader) {
    std::ifstream fileStream(std::filesystem::canonical(path_to_shader).string(), std::ios::binary);
    std::vector<char> buffer;
    buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
    return { (uint32_t*)buffer.data(), (uint32_t*)(buffer.data() + buffer.size()) };
}

template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

inline const std::integral auto subgroup_size(const kp::Manager& manager) {
    return manager.listDevices()
        .front()
        .getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceSubgroupProperties>()
        .get<vk::PhysicalDeviceSubgroupProperties>()
        .subgroupSize;
}

} // namespace tools
} // namespace pasimulations