
#pragma once

#include <chrono>
#include <concepts>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <thread>

#include <units/isq/si/length.h>
#include <units/isq/si/mass.h>
#include <units/isq/si/speed.h>

#include <gsl/gsl-lite.hpp>

#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>
#include <simulations/newton_point_simulation_2/NewtonPointSimulation2.hpp>
#include <tools.hpp>

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

namespace pasimulations {

enum class Distributions { normal_distribution };

enum class NewtonPointSimulationImplementations { cpu_1, gpu_1, gpu_2, gpu_3, gpu_4 };

template <std::integral I>
void run_newton_point_simulation_test(const NewtonPointSimulationImplementations implementation,
                                      const std::optional<I> optional_number_of_particles = std::optional<I> {},
                                      const std::optional<double> optional_seed = std::optional<double> {},
                                      const std::optional<I> optional_simulated_timesteps = std::optional<I> {},
                                      const bool disable_draw = false) {

    const auto number_of_particles = optional_number_of_particles.value_or(10000);
    const auto simulated_timesteps = optional_simulated_timesteps.value_or(1000);
    std::random_device rd;                          // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(optional_seed.value_or(rd())); // Standard mersenne_twister_engine seeded with rd()

    using namespace units;
    using namespace units::isq;

    auto simulator = nps::NewtonPointSimulation<float> {};

    using Real_vec = decltype(simulator)::Real_vec;
    auto testing_x_coords = Real_vec {};
    auto testing_y_coords = Real_vec {};
    auto testing_x_speeds = Real_vec {};
    auto testing_y_speeds = Real_vec {};
    auto testing_masses = Real_vec {};

    std::normal_distribution<> coordinate_radius_dist(3, 2);
    std::uniform_real_distribution<> coordinate_angle_dist(0.0, 2.0 * std::numbers::pi);
    std::uniform_real_distribution<> mass_dist(1.0, 1.3);

    std::normal_distribution<> velocity_magnitude_dist(10.0, 0.1);
    std::normal_distribution<> velocity_angle_dist(0.0, 0.0001);

    for ([[maybe_unused]] auto dump : std::ranges::iota_view(0, number_of_particles)) {
        const auto coordinate_radius = coordinate_radius_dist(gen);
        const auto coordinate_angle = coordinate_angle_dist(gen);
        testing_x_coords.push_back(coordinate_radius * std::cos(coordinate_angle));
        testing_y_coords.push_back(coordinate_radius * std::sin(coordinate_angle));

        const auto velocity_magnitude = velocity_magnitude_dist(gen);
        const auto velocity_angle = coordinate_angle + 0.5 * std::numbers::pi + velocity_angle_dist(gen);
        testing_x_speeds.push_back(velocity_magnitude * std::cos(velocity_angle));
        testing_y_speeds.push_back(velocity_magnitude * std::sin(velocity_angle));
        // testing_x_speeds.push_back(0.0);
        // testing_y_speeds.push_back(0.0);
        testing_masses.push_back(mass_dist(gen));
    }
    simulator.set_x_coordinates_from_reals(testing_x_coords);
    simulator.set_y_coordinates_from_reals(testing_y_coords);
    simulator.set_x_speeds_from_reals(testing_x_speeds);
    simulator.set_y_speeds_from_reals(testing_y_speeds);
    simulator.set_masses_from_reals(testing_masses);
    simulator.set_timestep_from_real(0.001);

    for ([[maybe_unused]] const auto _ : std::ranges::iota_view(0, simulated_timesteps)) {
        simulator.start_clock();
        switch (implementation) {
        case NewtonPointSimulationImplementations::cpu_1:
            simulator.evolve_with_cpu_1();
            break;
        case NewtonPointSimulationImplementations::gpu_1:
            simulator.evolve_with_gpu_1();
            break;
        case NewtonPointSimulationImplementations::gpu_2:
            simulator.evolve_with_gpu_2();
            break;
        case NewtonPointSimulationImplementations::gpu_3:
            simulator.evolve_with_gpu_3();
            break;
        case NewtonPointSimulationImplementations::gpu_4:
            simulator.evolve_with_gpu_4();
            break;
        }
        simulator.stop_clock();
        if (!disable_draw) {
            simulator.draw();
        } else {
            simulator.print_one_line_info();
        }
    }
}

enum class NewtonPointSimulation2Integrators { euler };

template <std::integral I, units::isq::Length L, units::isq::Speed V, units::isq::Mass M>
void run_newton_point_simulation_2(
    [[maybe_unused]] const NewtonPointSimulation2Integrators integrator,
    const std::optional<Distributions> optional_initial_position_distribution = std::optional<Distributions> {},
    const std::optional<Distributions> optional_initial_velocity_distribution = std::optional<Distributions> {},
    const std::optional<Distributions> optional_initial_mass_distribution = std::optional<Distributions> {},
    const std::optional<L> optional_initial_position_standard_deviation = std::optional<L> {},
    const std::optional<V> optional_initial_velocity_standard_deviation = std::optional<V> {},
    const std::optional<M> optional_initial_mass_standard_deviation = std::optional<M> {},
    const std::optional<M> optional_initial_mass_mean = std::optional<M> {},
    const std::optional<I> optional_number_of_particles = std::optional<I> {},
    const std::optional<double> optional_seed = std::optional<double> {}) {

    using namespace units;
    using namespace units::isq;

    using LengthUnit = si::metre;
    using SpeedUnit = si::metre_per_second;
    using MassUnit = si::kilogram;
    //    using TimeUnit = si::second;

    const auto initial_position_distribution =
        optional_initial_position_distribution.value_or(Distributions::normal_distribution);
    const auto initial_velocity_distribution =
        optional_initial_velocity_distribution.value_or(Distributions::normal_distribution);
    const auto initial_mass_distribution =
        optional_initial_mass_distribution.value_or(Distributions::normal_distribution);

    const auto initial_position_standard_deviation =
        optional_initial_position_standard_deviation.value_or(si::length<LengthUnit> { 1.0 });
    const auto initial_velocity_standard_deviation =
        optional_initial_velocity_standard_deviation.value_or(si::speed<SpeedUnit> { 1.0 });
    const auto initial_mass_standard_deviation =
        optional_initial_mass_standard_deviation.value_or(si::mass<MassUnit> { 1.0 });

    const auto initial_mass_mean = quantity_cast<MassUnit>(
        optional_initial_mass_mean.value_or(si::mass<MassUnit> { initial_mass_standard_deviation * 10.0 }));

    const auto number_of_particles = optional_number_of_particles.value_or(10000);

    std::random_device rd;                          // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(optional_seed.value_or(rd())); // Standard mersenne_twister_engine seeded with rd()
    auto initial_x_coordinates = std::vector<si::length<LengthUnit>> {};
    auto initial_y_coordinates = std::vector<si::length<LengthUnit>> {};
    auto initial_z_coordinates = std::vector<si::length<LengthUnit>> {};
    auto initial_x_speeds = std::vector<si::speed<SpeedUnit>> {};
    auto initial_y_speeds = std::vector<si::speed<SpeedUnit>> {};
    auto initial_z_speeds = std::vector<si::speed<SpeedUnit>> {};
    auto initial_masses = std::vector<si::mass<MassUnit>> {};

    std::uniform_real_distribution<> zero_to_pi_distribution(0.0, std::numbers::pi);

    switch (initial_position_distribution) {
    case Distributions::normal_distribution: {

        std::normal_distribution<> position_normal_distribution(0.0, initial_position_standard_deviation.number());
        pasimulations::tools::repeate_n_times(number_of_particles, [&]() noexcept {
            const auto theta = zero_to_pi_distribution(gen);
            const auto psi = 2.0 * zero_to_pi_distribution(gen);
            /* Calling absolute for this is unnecceary */
            const auto radius = si::length<LengthUnit> { position_normal_distribution(gen) };

            const auto [x, y, z] =
                pasimulations::tools::vec3<si::length<LengthUnit>>::create_from_polar_coordinates(theta, psi, radius);
            initial_x_coordinates.push_back(x);
            initial_y_coordinates.push_back(y);
            initial_z_coordinates.push_back(z);
        });
    }
    }

    switch (initial_velocity_distribution) {
    case Distributions::normal_distribution: {
        std::normal_distribution<> velocity_magnitude_normal_distribution(0.0,
                                                                          initial_velocity_standard_deviation.number());
        pasimulations::tools::repeate_n_times(number_of_particles, [&]() noexcept {
            const auto theta = zero_to_pi_distribution(gen);
            const auto psi = 2.0 * zero_to_pi_distribution(gen);
            /* Calling absolute for this is unnecceary */
            const auto magnitude = si::speed<SpeedUnit> { velocity_magnitude_normal_distribution(gen) };

            const auto [x_vel, y_vel, z_vel] =
                pasimulations::tools::vec3<si::speed<SpeedUnit>>::create_from_polar_coordinates(theta, psi, magnitude);
            initial_x_speeds.push_back(x_vel);
            initial_y_speeds.push_back(y_vel);
            initial_z_speeds.push_back(z_vel);
        });
    }
    }

    switch (initial_mass_distribution) {
    case Distributions::normal_distribution: {
        std::normal_distribution<> mass_normal_distribution(initial_mass_mean.number(),
                                                            initial_mass_standard_deviation.number());
        pasimulations::tools::repeate_n_times(number_of_particles, [&]() noexcept {
            const auto mass = si::mass<MassUnit> { std::abs(mass_normal_distribution(gen)) };

            initial_masses.push_back(mass);
        });
    }
    }

    auto simulation = pasimulations::nps::NewtonPointSimulation2<double> {};
    simulation.stage_initial_conditions_to_tensors(std::move(initial_x_coordinates), std::move(initial_y_coordinates),
                                        std::move(initial_z_coordinates), std::move(initial_x_speeds),
                                        std::move(initial_y_speeds), std::move(initial_z_speeds),
                                        std::move(initial_masses));
}

} // namespace pasimulations
