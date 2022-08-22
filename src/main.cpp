#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <cxxopts.hpp>
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <ANSI.hpp>
#include <pasimulations.hpp>
#include <tools.hpp>

/* 
    We resolve positional cl options in main.cpp and pass them froward,
    because it is assumed that without one program can not be run.

    Other options are made into optionals, because working with default 
    and implicite values is not nice.
 */

int main(int argc, char* argv[]) {
    try {
        
        auto options = cxxopts::Options("pasimulations", "Collection of simulations");

        // This needs to be held up to date manually
        const auto simulation_names_for_help =
            std::vector<std::string> { "newton_point_simulation" }; // Can be made consexpr in gcc11

        // clang-format off
        options
        .positional_help("simulation runtype")
        .set_width(70)
        .set_tab_expansion()
        .show_positional_help()
        .add_options()
        ("h,help", "Print usage") // Default values of are not used but needed  to not have errors when their options are not given
        ("number_of_particles", "Number of particles (points) in a simulation", cxxopts::value<int>()->default_value("0"))
        ("seed", "Seed used for rng", cxxopts::value<double>()->default_value("0"))
        ("timesteps", "This many timesteps will be simulated", cxxopts::value<int>()->default_value("0"))
        ("simulation", fmt::format("Available simulations: {}", fmt::join(simulation_names_for_help, ", ")), cxxopts::value<std::string>()->default_value(""))
        ("runtype", "Start a simulation with spesific runtype. To see available runtypes (eg. cpu_1) use list", cxxopts::value<std::string>()->default_value(""))
        ;
        // clang-format on

        options.parse_positional({ "simulation", "runtype" });

        const auto result = options.parse(argc, argv);

        auto parse_optional_option = [&]<typename T>(const std::string option, const T) {
            return result[option].count() ? std::optional<T> { result[option].as<T>() } : std::optional<T> {};
        };

        const auto number_of_particles = parse_optional_option("number_of_particles", int {});
        const auto seed = parse_optional_option("seed", double {});
        const auto timesteps = parse_optional_option("timesteps", int {});

        auto print_not_implemented_error = [&](const std::string what_is_missing,
                                               const std::vector<std::string> what_are_available) {
            const auto error_font = ansi::str(ansi::fg_brightred) + ansi::str(ansi::bold);

            fmt::print("{}{}{} is not implemented\n", error_font, result[what_is_missing].as<std::string>(),
                       ansi::str(ansi::reset));
            fmt::print("Following runtypes available:\n");
            for (const auto& runtype : what_are_available) {
                fmt::print("\t{}\n", runtype);
            }
        };

        if (result.count("help") || !(result["simulation"].count() && result["runtype"].count())) {
            fmt::print(options.help());
        } else {
            switch (pasimulations::tools::hash(result["simulation"].as<std::string>())) {
            case pasimulations::tools::hash("newton_point_simulation"): {
                std::optional<pasimulations::Newton_point_simulation_implementations> implementation {};

                switch (pasimulations::tools::hash(result["runtype"].as<std::string>())) {
                case pasimulations::tools::hash("cpu_1"): {
                    implementation = std::make_optional(pasimulations::Newton_point_simulation_implementations::cpu_1);

                } break;
                case pasimulations::tools::hash("gpu_1"): {
                    implementation = std::make_optional(pasimulations::Newton_point_simulation_implementations::gpu_1);
                } break;
                case pasimulations::tools::hash("gpu_2"): {
                    implementation = std::make_optional(pasimulations::Newton_point_simulation_implementations::gpu_2);
                } break;
                case pasimulations::tools::hash("gpu_3"): {
                    implementation = std::make_optional(pasimulations::Newton_point_simulation_implementations::gpu_3);
                } break;
                }

                if (implementation.has_value()) {
                    pasimulations::run_newton_point_simulation_test(implementation.value(), number_of_particles, seed,
                                                                    timesteps);
                } else {
                    print_not_implemented_error("runtype", { "cpu_1", "gpu_1", "gpu_2", "gpu_3" });
                }
            } break;
            default: {
                print_not_implemented_error("simulation", simulation_names_for_help);
            } break;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
