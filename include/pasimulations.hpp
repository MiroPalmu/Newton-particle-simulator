#pragma once

#include <chrono>
#include <concepts>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <thread>

#include <gsl/gsl-lite.hpp>

#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

namespace pasimulations {

enum class Newton_point_simulation_implementations { cpu_1, gpu_1, gpu_2 };

template <std::integral I>
void run_newton_point_simulation_test(const Newton_point_simulation_implementations implementation,
                                      const std::optional<I> optional_number_of_particles = std::optional<I> {},
                                      const std::optional<double> seed = std::optional<double> {},
                                      const std::optional<I> optional_simulated_timesteps = std::optional<I> {}) {

    const auto number_of_particles = optional_number_of_particles.value_or(1500);
    const auto simulated_timesteps = optional_simulated_timesteps.value_or(1000);
    std::random_device rd;                 // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(seed.value_or(rd())); // Standard mersenne_twister_engine seeded with rd()

    using namespace units;
    using namespace units::isq;

    auto simulator = nps::NewtonPointSimulation<double> {};

    using Real_vec = decltype(simulator)::Real_vec;
    auto testing_x_coords = Real_vec {};
    auto testing_y_coords = Real_vec {};
    auto testing_x_speeds = Real_vec {};
    auto testing_y_speeds = Real_vec {};
    auto testing_masses = Real_vec {};

    std::normal_distribution<> coordinate_radius_dist(3, 2);
    std::uniform_real_distribution<> coordinate_angle_dist(0.0, 2.0 * std::numbers::pi);
    std::normal_distribution<> velocity_magnitude_dist(10.0, 0.1);
    std::normal_distribution<> velocity_angle_dist(0.0, 0.0001);
    std::uniform_real_distribution<> mass_dist(1.0, 1.3);

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
        case Newton_point_simulation_implementations::cpu_1:
            simulator.evolve_with_cpu_1();
            break;
        case Newton_point_simulation_implementations::gpu_1:
            simulator.evolve_with_gpu_1();
            break;
        case Newton_point_simulation_implementations::gpu_2:
            simulator.evolve_with_gpu_2();
            break;
        }
        simulator.stop_clock();
        simulator.draw();
    }
}
} // namespace pasimulations
