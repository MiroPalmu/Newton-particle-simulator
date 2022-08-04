#pragma once

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
};
} // namespace tools
} // namespace pasimulations