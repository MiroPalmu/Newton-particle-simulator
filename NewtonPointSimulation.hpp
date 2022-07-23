#pragma once
#define FMT_HEADER_ONLY

#include "fmt/format.h"
#include "units/isq/si/acceleration.h"
#include "units/isq/si/constants.h"
#include "units/isq/si/force.h"
#include "units/isq/si/length.h"
#include "units/isq/si/mass.h"
#include "units/isq/si/time.h"
#include "units/quantity_io.h"
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

namespace nps {
using namespace units;
using namespace units::isq;

template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

/*
Stores data as units from template arguments.
Primitive data type is defnied in units.hpp
 */
template <UnitOf<si::dim_length> coordinate_unit, UnitOf<si::dim_mass> mass_unit, UnitOf<si::dim_time> time_unit,
          UnitOf<si::dim_speed> speed_unit, UnitOf<si::dim_acceleration> acceleration_unit>
class NewtonPointSimulation {

  private:
    std::vector<si::length<coordinate_unit>> x_coordinates_ {};
    std::vector<si::length<coordinate_unit>> y_coordinates_ {};
    std::vector<si::speed<speed_unit>> x_speeds_ {};
    std::vector<si::speed<speed_unit>> y_speeds_ {};
    std::vector<si::mass<mass_unit>> masses_ {};
    si::time<time_unit> simulation_time_ { 0.0 };

    si::time<time_unit> timestep_ { 1.0 };

    static constexpr auto G_units_ = si::length<si::metre> { 1 } * si::length<si::metre> { 1 } *
                                     si::length<si::metre> { 1 } / si::mass<si::kilogram> { 1 } /
                                     si::time<si::second> { 1 } / si::time<si::second> { 1 };
    static constexpr dimensionless<one> G_dimensioless_ { 1.0e-2 }; // Real value: 6.6743e-11

  public:
    void set_x_coordinates_from_doubles(const std::vector<double>& raw_x_coordniates) {
        x_coordinates_.clear();
        for (const auto x_raw : raw_x_coordniates) {
            x_coordinates_.emplace_back(si::length<coordinate_unit>(x_raw));
        }
    }

    void set_y_coordinates_from_doubles(const std::vector<double>& raw_y_coordniates) {
        y_coordinates_.clear();
        for (const auto y_raw : raw_y_coordniates) {
            y_coordinates_.emplace_back(si::length<coordinate_unit>(y_raw));
        }
    }

    void set_x_speeds_from_doubles(const std::vector<double>& raw_x_speeds) {
        x_speeds_.clear();
        for (const auto v_x_raw : raw_x_speeds) {
            x_speeds_.emplace_back(si::speed<speed_unit>(v_x_raw));
        }
    }

    void set_y_speeds_from_doubles(const std::vector<double>& raw_y_speeds) {
        y_speeds_.clear();
        for (const auto v_y_raw : raw_y_speeds) {
            y_speeds_.emplace_back(si::speed<speed_unit>(v_y_raw));
        }
    }

    void set_masses_from_doubles(const std::vector<double>& raw_mass) {
        masses_.clear();
        for (const auto mass_raw : raw_mass) {
            masses_.emplace_back(si::mass<mass_unit>(mass_raw));
        }
    }

    void set_timestep_from_double(const double timestep) { timestep_ = si::time<time_unit> { timestep }; }

    void print_info_of_particle(size_t i) {
        std::cout << "i: " << i << "\nmass: " << masses_[i] << "\n";
        std::cout << "x: " << x_coordinates_[i] << " " << x_speeds_[i] << "\n";
        std::cout << "y: " << y_coordinates_[i] << " " << y_speeds_[i] << "\n";
    }

    void draw(si::length<coordinate_unit> x_min = si::length<coordinate_unit>(si::length<coordinate_unit> { -10.0 }),
              si::length<coordinate_unit> x_max = si::length<coordinate_unit>(si::length<coordinate_unit> { 10.0 }),
              si::length<coordinate_unit> y_min = si::length<coordinate_unit>(si::length<coordinate_unit> { -10.0 }),
              si::length<coordinate_unit> y_max = si::length<coordinate_unit>(si::length<coordinate_unit> { 10.0 })) {
        // height / width
        constexpr auto aspect_ratio = 2.0;

        const auto width = x_max - x_min;
        const auto height = y_max - y_min;

        const auto width_in_pixels = aspect_ratio * width.number();
        const auto height_in_pixels = height.number();

        auto frame = std::vector<std::string>(width_in_pixels * height_in_pixels, " ");

        const auto particles = x_coordinates_.size();
        for (size_t i { 0 }; i < particles; ++i) {
            const auto x_screen_pos = (x_coordinates_[i] - x_min) / width;
            const auto y_screen_pos = (y_coordinates_[i] - y_min) / height;
            if (x_screen_pos.number() > 0 && x_screen_pos.number() < 1 && y_screen_pos.number() > 0 &&
                y_screen_pos.number() < 1) {
                const auto x_index = int(width_in_pixels * x_screen_pos.number());
                const auto y_index = int(height_in_pixels * y_screen_pos.number());

                frame[x_index + width_in_pixels * y_index] = "X";
            }
        }

        for (size_t i { 0 }; i < height_in_pixels; ++i) {
            for (size_t j { 0 }; j < width_in_pixels; ++j) {
                fmt::print(frame[j + i * width_in_pixels]);
            }
            fmt::print("\n");
        }
        fmt::print("\033[{}A", height_in_pixels);
    }

    // The most basic implementation
    void evolve_with_cpu_1() {
        /*
                std::cout << x_coordinates_.size() << "  " << y_coordinates_.size() << "  " << x_speeds_.size() << "  "
                          << y_speeds_.size() << "  " << masses_.size() << "\n";
         */
        assert(x_coordinates_.size() == y_coordinates_.size() && x_coordinates_.size() == x_speeds_.size() &&
               x_coordinates_.size() == y_speeds_.size() && x_coordinates_.size() == masses_.size());

        const auto particles = x_coordinates_.size();

        auto x_accelerations =
            std::vector<si::acceleration<acceleration_unit>>(particles, si::acceleration<acceleration_unit> { 0.0 });
        auto y_accelerations =
            std::vector<si::acceleration<acceleration_unit>>(particles, si::acceleration<acceleration_unit> { 0.0 });

        for (size_t i { 0 }; i < particles - 1; ++i) {
            for (size_t j { i + 1 }; j < particles; ++j) {
                const Length auto d_x = x_coordinates_[j] - x_coordinates_[i];
                const Length auto d_y = y_coordinates_[j] - y_coordinates_[i];
                const auto d2 = d_x * d_x + d_y * d_y;
                // G units business feels little jank
                // Units are restricting optimizations and even trying to do them
                // (hoping that G_units_ * gets optimized away by compiler)
                // we just go around the things that units were supposed to do.

                // This is also evading the purpose of units library
                if (true || d2.number() > 0.2) {

                    const auto d_x_hat = sign(d_x.number());
                    const auto d_y_hat = sign(d_y.number());
                    x_accelerations[i] += d_x_hat * G_units_ * masses_[j] / d2;
                    y_accelerations[i] += d_y_hat * G_units_ * masses_[i] / d2;
                    x_accelerations[j] -= d_x_hat * G_units_ * masses_[j] / d2;
                    y_accelerations[j] -= d_y_hat * G_units_ * masses_[i] / d2;
                }
            }
        }
        for (size_t i { 0 }; i < particles; ++i) {
            x_accelerations[i] *= G_dimensioless_;
            y_accelerations[i] *= G_dimensioless_;
        }

        // New velocity                     // Feels jank
        auto calculate_speed_deltas = [&](decltype(x_accelerations)& acceleration) {
            std::vector<si::speed<speed_unit>> speed_deltas(particles);
            for (size_t i { 0 }; i < particles; ++i) {
                speed_deltas[i] = acceleration[i] * timestep_;
            }
            return speed_deltas;
        };

        const auto x_speed_deltas = calculate_speed_deltas(x_accelerations);
        const auto y_speed_deltas = calculate_speed_deltas(y_accelerations);

        for (size_t i { 0 }; i < particles; ++i) {
            x_speeds_[i] += x_speed_deltas[i];
            y_speeds_[i] += y_speed_deltas[i];

            // New position from new velocity
            x_coordinates_[i] += x_speeds_[i] * timestep_;
            y_coordinates_[i] += y_speeds_[i] * timestep_;
        }
    }
};

} // namespace nps