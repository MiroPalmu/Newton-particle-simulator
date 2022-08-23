#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <ANSI.hpp>

#include <tools.hpp>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <units/isq/si/constants.h>
#include <units/isq/si/force.h>
#include <units/isq/si/length.h>
#include <units/isq/si/mass.h>
#include <units/isq/si/time.h>
#include <units/quantity_io.h>

#include <gsl/gsl-lite.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define KOMPUTE_LOG_LEVEL 5
#include "../libs/kompute/single_include/kompute/Kompute.hpp"
#pragma GCC diagnostic pop

namespace pasimulations {

namespace nps {
using namespace units;
using namespace units::isq;

template <std::floating_point Real>
class NewtonPointSimulation {
    // We assume that everything is in SI units in NewtonPointSimulation
  public:
    using Real_vec = std::vector<Real>;

  private:
    Real_vec x_coordinates_ {};
    Real_vec y_coordinates_ {};
    Real_vec x_speeds_ {};
    Real_vec y_speeds_ {};
    Real_vec masses_ {};
    Real simulation_time_ { 0.0 };

    Real timestep_ { 1 };

    Real softening_radius_ { 0.1 };

    Real G_ { 0.34 }; // Real value: 6.6743e-11

    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    time_point timing_clock_;
    std::array<std::chrono::milliseconds, 5> last_n_clocked_times_ {};

    static constexpr Real default_draw_area_side_length_ { 10.0 };

  public:
    /*
        This is the unsafe way to load data to simulation.
        There should be version of these that uses mp-units as
        typesafe interface to set data. After the data is inserted
        inside the simulation we can assume that the values will stay
        in SI units inside the simulation.

        Example of this is draw function.
     */
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

    void set_x_coordinates_from_reals(const Real_vec& x_coordinates) { x_coordinates_ = x_coordinates; }
    void set_y_coordinates_from_reals(const Real_vec& y_coordinates) { y_coordinates_ = y_coordinates; }
    void set_x_speeds_from_reals(const Real_vec& x_speeds) { x_speeds_ = x_speeds; }
    void set_y_speeds_from_reals(const Real_vec& y_speeds) { y_speeds_ = y_speeds; }
    void set_masses_from_reals(const Real_vec& masses) { masses_ = masses; }
    void set_timestep_from_real(const Real timestep) { timestep_ = timestep; }
    void set_G_from_real(const Real G) { G_ = G; }

    void print_one_line_info() {
        const auto particles = x_coordinates_.size();
        fmt::print("n: {}, Simulation time: {:>5.2f}s, Wall clock of timestep: {:>4}ms", particles, simulation_time_,
                   calculation_time_average_().count());

        fmt::print("{}", ansi::str(ansi::cursorhoriz(0)));
    }
    

    void print_info_of_particle(gsl::index i) {
        std::cout << "i: " << i << "\nmass: " << masses_[i] << "\n";
        std::cout << "x: " << x_coordinates_[i] << " " << x_speeds_[i] << "\n";
        std::cout << "y: " << y_coordinates_[i] << " " << y_speeds_[i] << "\n";
    }

    void draw /* using default draw window */ () {
        auto x_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 };
        auto x_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 };
        auto y_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 };
        auto y_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 };

        draw(x_min, x_max, y_min, y_max);
    };
    void draw(Length auto x_min_with_units, Length auto x_max_with_units, Length auto y_min_with_units,
              Length auto y_max_with_units) {
        const auto x_min = static_cast<Real>(quantity_cast<si::length<si::metre>>(x_min_with_units).number());
        const auto x_max = static_cast<Real>(quantity_cast<si::length<si::metre>>(x_max_with_units).number());
        const auto y_min = static_cast<Real>(quantity_cast<si::length<si::metre>>(y_min_with_units).number());
        const auto y_max = static_cast<Real>(quantity_cast<si::length<si::metre>>(y_max_with_units).number());

        const auto width = x_max - x_min;
        const auto height = y_max - y_min;

        static constexpr auto aspect_ratio /* = width / height */ = 2.0;
        static constexpr auto height_in_pixels = 50;
        static constexpr auto width_in_pixels = int(std::round(height_in_pixels * aspect_ratio));

        auto frame = std::vector<std::string>(width_in_pixels * height_in_pixels, " ");

        const auto particles = x_coordinates_.size();
        for (size_t i { 0 }; i < particles; ++i) {
            const auto x_screen_pos = (x_coordinates_[i] - x_min) / width;
            const auto y_screen_pos = (y_coordinates_[i] - y_min) / height;
            /* Test if we are in frame */
            if (x_screen_pos > 0 && x_screen_pos < 1 && y_screen_pos > 0 && y_screen_pos < 1) {
                const auto x_index = int(width_in_pixels * x_screen_pos);
                const auto y_index = int(height_in_pixels * y_screen_pos);

                const auto ansi_color_code_beginning =
                    std::string { "\033[38;5;" } + std::to_string(i / 255) + std::string { "m" };
                static auto ansi_color_code_ending = ansi::str(ansi::reset);
                // Test if there is special effects already on this pixel
                if (frame[x_index + width_in_pixels * y_index] != "O") {
                    frame[x_index + width_in_pixels * y_index] =
                        ansi_color_code_beginning + "O" + ansi_color_code_ending;
                }
            }
        }

        for (size_t i { 0 }; i < height_in_pixels; ++i) {
            for (size_t j { 0 }; j < width_in_pixels; ++j) {
                std::cout << frame[j + i * width_in_pixels];
            }
            fmt::print("\n");
        }

        fmt::print("{}\r", ansi::str(ansi::clrline()));

        print_one_line_info();

        fmt::print("{}", ansi::str(ansi::cursorup(height_in_pixels)));
    }

    /*
        Properties:
        * Most basic implementation
     */
    void evolve_with_cpu_1() {

        assert(x_coordinates_.size() == y_coordinates_.size() && x_coordinates_.size() == x_speeds_.size() &&
               x_coordinates_.size() == y_speeds_.size() && x_coordinates_.size() == masses_.size());

        const auto particles = static_cast<int64_t>(x_coordinates_.size());

        auto x_accelerations = Real_vec(particles, 0.0);
        auto y_accelerations = Real_vec(particles, 0.0);

        for (const auto i : std::ranges::iota_view(0, particles)) {
            for (const auto j : std::ranges::iota_view(0, particles)) {

                const auto d_x = x_coordinates_[j] - x_coordinates_[i];
                const auto d_y = y_coordinates_[j] - y_coordinates_[i];
                /*
                    We calculate this way because sign function is problematic
                    Results might differ between cpu and gpu runs
                 */
                const auto d = std::sqrt(d_x * d_x + d_y * d_y + softening_radius_ * softening_radius_);

                x_accelerations[i] += d_x * G_ * masses_[j] / (d * d * d);
                y_accelerations[i] += d_y * G_ * masses_[j] / (d * d * d);
            }
            y_speeds_[i] += y_accelerations[i] * timestep_;
            x_speeds_[i] += x_accelerations[i] * timestep_;
        }
        // New position from new velocity
        for (const auto i : std::ranges::iota_view(0, particles)) {
            x_coordinates_[i] += x_speeds_[i] * timestep_;
            y_coordinates_[i] += y_speeds_[i] * timestep_;
        }
        simulation_time_ += timestep_;
    }

    /*
    Properties:
    * Most basic implementation
     */
    void evolve_with_gpu_1();

    /*
    Properties:
    * Most basic implementation
    * Set size of workgroups to 64 and added padding
     */
    void evolve_with_gpu_2();

    /*
    Properties:
    * Most basic implementation
    * Set size of workgroups to 64 and added padding
    * Set workgropu size to 64 (and used some build-in functions)
     */
    void evolve_with_gpu_3();

    void evolve_with_gpu_4();
};

} // namespace nps
} // namespace pasimulations