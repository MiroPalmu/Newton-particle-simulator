#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define KOMPUTE_LOG_LEVEL 5
#include "../libs/kompute/single_include/kompute/Kompute.hpp"
#pragma GCC diagnostic pop

// This is just to suppress warings
#include ".vscode/suppress_intellisense_warnings.hpp"

#define FMT_HEADER_ONLY
#include "ANSI.hpp"
#include "fmt/format.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "NewtonPointSimulation.hpp"
#include <chrono>
#include <random>
#include <thread>

// Forward declaring our helper function to read compiled shader
static std::vector<uint32_t>
readShader(const std::string shader_path);
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {

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
    std::uniform_real_distribution<> coordinate_dist(-5.0, 5.0);
    std::uniform_real_distribution<> speed_dist(-0.1, 0.1);
    std::uniform_real_distribution<> mass_dist(1.0, 1.1);
    for (size_t i { 0 }; i < 5000; ++i) {
        testing_x_coords.push_back(coordinate_dist(gen));
        testing_y_coords.push_back(coordinate_dist(gen));
        testing_x_speeds.push_back(speed_dist(gen));
        testing_y_speeds.push_back(speed_dist(gen));
        // testing_x_speeds.push_back(0.0);
        // testing_y_speeds.push_back(0.0);
        testing_masses.push_back(mass_dist(gen));
    }
    simulator.set_x_coordinates_from_doubles(testing_x_coords);
    simulator.set_y_coordinates_from_doubles(testing_y_coords);
    simulator.set_x_speeds_from_doubles(testing_x_speeds);
    simulator.set_y_speeds_from_doubles(testing_y_speeds);
    simulator.set_masses_from_doubles(testing_masses);
    simulator.set_timestep_from_double(0.1);

    for (size_t i { 0 }; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        simulator.start_clock();
        simulator.evolve_with_cpu_1();
        simulator.stop_clock();
        simulator.draw();
    }
}

[[maybe_unused]] static std::vector<uint32_t> readShader(const std::string shader_path) {
    std::ifstream fileStream(shader_path, std::ios::binary);
    std::vector<char> buffer;
    buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
    return { (uint32_t*)buffer.data(), (uint32_t*)(buffer.data() + buffer.size()) };
}
