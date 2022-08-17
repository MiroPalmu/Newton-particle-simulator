#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
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

    static constexpr Real softeing_radius { 0.1 };

    static constexpr Real G_ { 0.34 }; // Real value: 6.6743e-11

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

    void print_info_of_particle(gsl::index i) {
        std::cout << "i: " << i << "\nmass: " << masses_[i] << "\n";
        std::cout << "x: " << x_coordinates_[i] << " " << x_speeds_[i] << "\n";
        std::cout << "y: " << y_coordinates_[i] << " " << y_speeds_[i] << "\n";
    }
    /*
    auto x_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 }
    auto x_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 }
    auto y_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 }
    auto y_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 } */

    void draw /* using default draw window */ () {
        auto x_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 };
        auto x_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 };
        auto y_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 };
        auto y_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 };

        draw(x_min, x_max, y_min, y_max);
    };
    void draw(Length auto x_min_with_units, Length auto x_max_with_units, Length auto y_min_with_units,
              Length auto y_max_with_units) {
        const auto x_min = Real { quantity_cast<si::length<si::metre>>(x_min_with_units).number() };
        const auto x_max = Real { quantity_cast<si::length<si::metre>>(x_max_with_units).number() };
        const auto y_min = Real { quantity_cast<si::length<si::metre>>(y_min_with_units).number() };
        const auto y_max = Real { quantity_cast<si::length<si::metre>>(y_max_with_units).number() };

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
                if (masses_[i] < Real { 2.0 } && frame[x_index + width_in_pixels * y_index] != "X") {
                    frame[x_index + width_in_pixels * y_index] =
                        ansi_color_code_beginning + "X" + ansi_color_code_ending;
                } else {
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

        fmt::print("n: {}, Simulation time: {:>5.2f}s, Wall clock of timestep: {:>4}ms", particles, simulation_time_,
                   calculation_time_average_().count());

        fmt::print("{}{}", ansi::str(ansi::cursorhoriz(0)), ansi::str(ansi::cursorup(height_in_pixels)));
    }

    // The most basic implementation
    void evolve_with_cpu_1() {
        simulation_time_ += timestep_;
        /*
                std::cout << x_coordinates_.size() << "  " << y_coordinates_.size() << "  " << x_speeds_.size() << "  "
                          << y_speeds_.size() << "  " << masses_.size() << "\n";
         */
        assert(x_coordinates_.size() == y_coordinates_.size() && x_coordinates_.size() == x_speeds_.size() &&
               x_coordinates_.size() == y_speeds_.size() && x_coordinates_.size() == masses_.size());

        const auto particles = x_coordinates_.size();

        auto x_accelerations = Real_vec(particles, 0.0);
        auto y_accelerations = Real_vec(particles, 0.0);

        for (gsl::index i { 0 }; i < particles - 1; ++i) {
            for (gsl::index j { i + 1 }; j < particles; ++j) {
                const std::floating_point auto d_x = x_coordinates_[j] - x_coordinates_[i];
                const std::floating_point auto d_y = y_coordinates_[j] - y_coordinates_[i];
                const std::floating_point auto d2 = d_x * d_x + d_y * d_y + softeing_radius * softeing_radius;

                const auto d_x_hat = pasimulations::tools::sign(d_x);
                const auto d_y_hat = pasimulations::tools::sign(d_y);
                x_accelerations[i] += d_x_hat * G_ * masses_[j] / d2;
                y_accelerations[i] += d_y_hat * G_ * masses_[i] / d2;
                x_accelerations[j] -= d_x_hat * G_ * masses_[j] / d2;
                y_accelerations[j] -= d_y_hat * G_ * masses_[i] / d2;
            }
        }

        for (gsl::index i { 0 }; i < particles; ++i) {
            x_speeds_[i] += x_accelerations[i] * timestep_;
            y_speeds_[i] += y_accelerations[i] * timestep_;

            // New position from new velocity
            x_coordinates_[i] += x_speeds_[i] * timestep_;
            y_coordinates_[i] += y_speeds_[i] * timestep_;
        }
    }

    void evolve_with_gpu_1();
};

} // namespace nps
} // namespace pasimulations