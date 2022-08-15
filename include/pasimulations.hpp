#pragma once

#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>
#include <chrono>
#include <concepts>
#include <numbers>
#include <random>
#include <thread>

namespace pasimulations {

enum class Newton_point_simulation_implementations { cpu_1, gpu_1 };

void run_newton_point_simulation_test(const std::integral auto number_of_particles,
                                      const Newton_point_simulation_implementations implementation) {
    using namespace units;
    using namespace units::isq;

    auto simulator =
        nps::NewtonPointSimulation<units::isq::si::metre, units::isq::si::kilogram, units::isq::si::second,
                                   units::isq::si::metre_per_second, units::isq::si::metre_per_second_sq> {};

    auto testing_x_coords = std::vector<double> {};
    auto testing_y_coords = std::vector<double> {};
    auto testing_x_speeds = std::vector<double> {};
    auto testing_y_speeds = std::vector<double> {};
    auto testing_masses = std::vector<double> {};

    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::normal_distribution<> coordinate_radius_dist(3, 2);
    std::uniform_real_distribution<> coordinate_angle_dist(0.0, 2.0 * std::numbers::pi);
    std::normal_distribution<> velocity_magnitude_dist(10.0, 0.1);
    std::normal_distribution<> velocity_angle_dist(0, 0.0001);
    std::uniform_real_distribution<> mass_dist(1.0, 1.0);
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
    simulator.set_x_coordinates_from_doubles(testing_x_coords);
    simulator.set_y_coordinates_from_doubles(testing_y_coords);
    simulator.set_x_speeds_from_doubles(testing_x_speeds);
    simulator.set_y_speeds_from_doubles(testing_y_speeds);
    simulator.set_masses_from_doubles(testing_masses);
    simulator.set_timestep_from_double(0.001);

    for (size_t i { 0 }; i < 10000; ++i) {
        if (number_of_particles < 1500) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        simulator.start_clock();
        switch (implementation) {
        case Newton_point_simulation_implementations::cpu_1:
            simulator.evolve_with_cpu_1();
            break;
        case Newton_point_simulation_implementations::gpu_1:
            simulator.evolve_with_gpu_1();
            break;
        }
        simulator.stop_clock();
        simulator.draw();
    }
}

} // namespace pasimulations
