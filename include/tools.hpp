#pragma once
#include <cmath>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

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

class TimerFunctionality {
  protected:
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    time_point timing_clock_;
    std::array<std::chrono::milliseconds, 5> last_n_clocked_times_ {};

  public:
    void start_clock() { timing_clock_ = std::chrono::steady_clock::now(); }

    void stop_clock() {
        const auto end_clock_time_ = std::chrono::steady_clock::now();

        std::ranges::rotate(last_n_clocked_times_, last_n_clocked_times_.begin() + 1);
        last_n_clocked_times_.back() =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_clock_time_ - timing_clock_);
    }

    std::chrono::milliseconds calculation_time_average_() {
        std::chrono::milliseconds sum_of_clocked_times {};

        for (auto time : last_n_clocked_times_) {
            sum_of_clocked_times += time;
        }

        const auto average_over_last_n_times_in_ms_ =
            double(sum_of_clocked_times.count()) / double(last_n_clocked_times_.size());

        return std::chrono::milliseconds(size_t(average_over_last_n_times_in_ms_));
    }
};

template <std::floating_point R, std::integral I>
[[nodiscard]] std::vector<R> generate_N_numbers_from_uniform_distributions(const I N, auto& generator,
                                                                           const R lower_bound = R { 0 },
                                                                           const R upper_bound = R { 1 }) {
    std::uniform_real_distribution<> uniform_distribution(lower_bound, upper_bound);
    auto numbers = std::vector(N);
    for (auto& element : numbers) {
        element = uniform_distribution(generator);
    }
    return numbers;
};

void repeate_n_times(std::integral auto n, const std::invocable auto& f) {
    for (auto amount = n; amount; --amount)
        [[likely]] { std::invoke(f); }
}

template <typename T, typename R>
concept scalar = std::floating_point<R> && requires(T x, R y) {
    { x* y } -> std::same_as<T>;
    { y* x } -> std::same_as<T>;
    { x + x } -> std::same_as<T>;
};

template <scalar<double> S>
struct vec3 {
    S x;
    S y;
    S z;

    // ISO 80000-2:2019 convention
    static constexpr auto create_from_polar_coordinates(const double theta, const double psi,
                                                        const S radius = 1.0) noexcept {
        return vec3 { radius * std::cos(psi) * std::sin(theta), radius * std::sin(psi) * std::sin(theta),
                      radius * std::cos(theta) };
    }
};

} // namespace tools
} // namespace pasimulations