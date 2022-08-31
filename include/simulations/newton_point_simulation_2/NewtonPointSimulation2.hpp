#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numeric>
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

#include <vulkan/vulkan_raii.hpp>

namespace pasimulations {

namespace nps {

using namespace units;
using namespace units::isq;

template <std::floating_point Real>
class NewtonPointSimulation2 : pasimulations::tools::TimerFunctionality {
    /*
        We assume that lengths and masses is in N-body units in NewtonPointSimulation2
        Everything else is in SI

        When givin data to nps2 it has to be staged first and then synced with gpu
    */
  public:
    using Real_vec = std::vector<Real>;

  private:
    const std::vector<std::string> desired_extensions { "GL_KHR_shader_subgroup_basic", "GL_ARB_gpu_shader_fp64",
                                                        "GL_KHR_shader_subgroup_shuffle" };
    kp::Manager manager = kp::Manager(0, {}, desired_extensions);

    std::shared_ptr<kp::TensorT<Real>> tensor_x_coordinates_ {};
    std::shared_ptr<kp::TensorT<Real>> tensor_y_coordinates_ {};
    std::shared_ptr<kp::TensorT<Real>> tensor_z_coordinates_ {};
    std::shared_ptr<kp::TensorT<Real>> tensor_x_speeds_ {};
    std::shared_ptr<kp::TensorT<Real>> tensor_y_speeds_ {};
    std::shared_ptr<kp::TensorT<Real>> tensor_z_speeds_ {};
    std::shared_ptr<kp::TensorT<Real>> tensor_masses_ {};

    size_t number_of_particles_ { 0 };

  public:
    /*
        This is the unsafe way to load data to simulation.
        There should be version of these that uses mp-units as
        typesafe interface to set data. After the data is inserted
        inside the simulation we can assume that the values will stay
        in SI units inside the simulation.

        Example of this is draw function.
     */

    /*
         These need to be checked how they will work in nps2
        (and maybe to move the defenitions to cpp, oh wait we cant because the class is template)
        we could only move the gpu calculations to cpp file because they are defined for spesific
        floating point type
    */

    template <Length L, Speed V, Mass M>
    void stage_initial_conditions(const std::vector<L>&& x_coordinates, const std::vector<L>&& y_coordinates,
                                  const std::vector<L>&& z_coordinates, const std::vector<V>&& x_speeds,
                                  const std::vector<V>&& y_speeds, const std::vector<V>&& z_speeds,
                                  const std::vector<M>&& masses) {
        assert(x_coordinates.size() == y_coordinates.size() == z_coordinates.size() == x_speeds.size() ==
               y_speeds.size() == z_speeds.size() == masses.size());
        /*
            We will first convert to work units to calculate the nbody units and then convert to those
         */

        number_of_particles_ = x_coordinates.size();
        const auto nbody_mass_unit_in_si =
            units::quantity_cast<si::kilogram>(
                std::accumulate(masses.begin(), masses.end(), si::mass<si::kilogram> { 0.0 }))
                .number();

        const auto nbody_length_unit_inverse_in_si = double { 0.0 };

        for (const auto i : std::ranges::iota_view(0, number_of_particles_ - 1)) {
            for (const auto j : std::ranges::iota_view(i + 1, number_of_particles_)) {
                const auto dx = units::quantity_cast<si::metre>(x_coordinates[i] - x_coordinates[j]).number();
                const auto dy = units::quantity_cast<si::metre>(y_coordinates[i] - y_coordinates[j]).number();
                const auto dz = units::quantity_cast<si::metre>(z_coordinates[i] - z_coordinates[j]).number();

                const auto dr = std::sqrt(dx * dx + dy * dy + dz * dz);

                nbody_length_unit_inverse_in_si +=
                    2.0 * units::quantity_cast<si::kilogram>(masses[i] * masses[j]).number() / dr;
            }
        }
        nbody_length_unit_inverse_in_si /= nbody_mass_unit_in_si * nbody_mass_unit_in_si;

        auto length_to_nbody_units = [&](const Length auto x) {
            return units::quantity_cast<si::metre>(x).number() * nbody_length_unit_inverse_in_si;
        };

        auto mass_to_nbody_units = [&](const Mass auto m) {
            return units::quantity_cast<si::kilogram>(m).number() / nbody_mass_unit_in_si;
        };

        auto speed_to_nbody_units = [&](const Speed auto v) {
            return units::quantity_cast<si::metre_per_second>(v).number() * nbody_length_unit_inverse_in_si;
        };

        auto x_coordinates_in_nbody_units = Real_vec(number_of_particles_);
        auto y_coordinates_in_nbody_units = Real_vec(number_of_particles_);
        auto z_coordinates_in_nbody_units = Real_vec(number_of_particles_);
        auto x_speeds_in_nbody_units = Real_vec(number_of_particles_);
        auto y_speeds_in_nbody_units = Real_vec(number_of_particles_);
        auto z_speeds_in_nbody_units = Real_vec(number_of_particles_);
        auto masses_in_nbody_units = Real_vec(number_of_particles_);

        std::ranges::transform(x_coordinates, x_coordinates_in_nbody_units.begin(), length_to_nbody_units);
        std::ranges::transform(y_coordinates, y_coordinates_in_nbody_units.begin(), length_to_nbody_units);
        std::ranges::transform(z_coordinates, z_coordinates_in_nbody_units.begin(), length_to_nbody_units);
        std::ranges::transform(x_speeds, x_speeds_in_nbody_units.begin(), speed_to_nbody_units);
        std::ranges::transform(y_speeds, y_speeds_in_nbody_units.begin(), speed_to_nbody_units);
        std::ranges::transform(z_speeds, z_speeds_in_nbody_units.begin(), speed_to_nbody_units);
        std::ranges::transform(masses, masses_in_nbody_units.begin(), mass_to_nbody_units);

        tensor_x_coordinates_.rebuild(x_coordinates.data(), number_of_particles_, sizeof(Real));
        tensor_y_coordinates_.rebuild(y_coordinates.data(), number_of_particles_, sizeof(Real));
        tensor_z_coordinates_.rebuild(z_coordinates.data(), number_of_particles_, sizeof(Real));
        tensor_x_speeds_.rebuild(x_speeds.data(), number_of_particles_, sizeof(Real));
        tensor_y_speeds_.rebuild(y_speeds.data(), number_of_particles_, sizeof(Real));
        tensor_z_speeds_.rebuild(z_speeds.data(), number_of_particles_, sizeof(Real));
        tensor_masses_.rebuild(masses.data(), number_of_particles_, sizeof(Real));
    }
    // void set_y_coordinates_from_reals(const Real_vec& y_coordinates) { y_coordinates_ = y_coordinates; }
    // void set_x_speeds_from_reals(const Real_vec& x_speeds) { x_speeds_ = x_speeds; }
    // void set_y_speeds_from_reals(const Real_vec& y_speeds) { y_speeds_ = y_speeds; }
    // void set_masses_from_reals(const Real_vec& masses) { masses_ = masses; }
    // void set_timestep_from_real(const Real timestep) { timestep_ = timestep; }
    // void set_G_from_real(const Real G) { G_ = G; }
    // void print_one_line_info() {
    //     const auto particles = x_coordinates_.size();
    //     fmt::print("n: {}, Simulation time: {:>5.2f}s, Wall clock of timestep: {:>4}ms", particles, simulation_time_,
    //                calculation_time_average_().count());

    //     fmt::print("{}", ansi::str(ansi::cursorhoriz(0)));
    // }

    // void print_info_of_particle(gsl::index i) {
    //     std::cout << "i: " << i << "\nmass: " << masses_[i] << "\n";
    //     std::cout << "x: " << x_coordinates_[i] << " " << x_speeds_[i] << "\n";
    //     std::cout << "y: " << y_coordinates_[i] << " " << y_speeds_[i] << "\n";
    // }

    // void draw /* using default draw window */ () {
    //     auto x_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 };
    //     auto x_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 };
    //     auto y_min = si::length<si::metre> { -default_draw_area_side_length_ / 2.0 };
    //     auto y_max = si::length<si::metre> { default_draw_area_side_length_ / 2.0 };

    //     draw(x_min, x_max, y_min, y_max);
    // };
    // void draw(Length auto x_min_with_units, Length auto x_max_with_units, Length auto y_min_with_units,
    //           Length auto y_max_with_units) {
    //     const auto x_min = static_cast<Real>(quantity_cast<si::length<si::metre>>(x_min_with_units).number());
    //     const auto x_max = static_cast<Real>(quantity_cast<si::length<si::metre>>(x_max_with_units).number());
    //     const auto y_min = static_cast<Real>(quantity_cast<si::length<si::metre>>(y_min_with_units).number());
    //     const auto y_max = static_cast<Real>(quantity_cast<si::length<si::metre>>(y_max_with_units).number());

    //     const auto width = x_max - x_min;
    //     const auto height = y_max - y_min;

    //     static constexpr auto aspect_ratio /* = width / height */ = 2.0;
    //     static constexpr auto height_in_pixels = 50;
    //     static constexpr auto width_in_pixels = int(std::round(height_in_pixels * aspect_ratio));

    //     auto frame = std::vector<std::string>(width_in_pixels * height_in_pixels, " ");

    //     const auto particles = x_coordinates_.size();
    //     for (size_t i { 0 }; i < particles; ++i) {
    //         const auto x_screen_pos = (x_coordinates_[i] - x_min) / width;
    //         const auto y_screen_pos = (y_coordinates_[i] - y_min) / height;
    //         /* Test if we are in frame */
    //         if (x_screen_pos > 0 && x_screen_pos < 1 && y_screen_pos > 0 && y_screen_pos < 1) {
    //             const auto x_index = int(width_in_pixels * x_screen_pos);
    //             const auto y_index = int(height_in_pixels * y_screen_pos);

    //             const auto ansi_color_code_beginning =
    //                 std::string { "\033[38;5;" } + std::to_string(i / 255) + std::string { "m" };
    //             static auto ansi_color_code_ending = ansi::str(ansi::reset);
    //             // Test if there is special effects already on this pixel
    //             if (frame[x_index + width_in_pixels * y_index] != "O") {
    //                 frame[x_index + width_in_pixels * y_index] =
    //                     ansi_color_code_beginning + "O" + ansi_color_code_ending;
    //             }
    //         }
    //     }

    //     for (size_t i { 0 }; i < height_in_pixels; ++i) {
    //         for (size_t j { 0 }; j < width_in_pixels; ++j) {
    //             std::cout << frame[j + i * width_in_pixels];
    //         }
    //         fmt::print("\n");
    //     }

    //     fmt::print("{}\r", ansi::str(ansi::clrline()));

    //     print_one_line_info();

    //     fmt::print("{}", ansi::str(ansi::cursorup(height_in_pixels)));
    // }
};

} // namespace nps
} // namespace pasimulations