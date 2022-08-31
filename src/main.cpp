#include <array>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <units/isq/si/length.h>
#include <units/isq/si/mass.h>
#include <units/isq/si/speed.h>

#include <cxxopts.hpp>
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <ANSI.hpp>
#include <pasimulations.hpp>
#include <tools.hpp>

/*
    We resolve positional cl options in main.cpp and pass them forward,
    because it is assumed that without one program can not be run.

    Other options are made into optionals, because working with default
    and implicite values of cxxopts is not nice.

    Except for boolean options where we give default value
 */

int main(int argc, char* argv[]) {
    try {

        auto options = cxxopts::Options("pasimulations", "Collection of simulations");

        // Therse needs to be held up to date manually
        const auto simulation_names_for_help =
            std::vector<std::string> { "newton_point_simulation",
                                       "newton_point_simulation_2" }; // Can be made consexpr in gcc11

        const auto distribution_names_for_help = std::vector<std::string> { "normal" };
        const auto distribution_help_text =
            fmt::format("Available distributions: {}", fmt::join(distribution_names_for_help, ", "));

        // clang-format off
        options
        .positional_help("simulation runtype")
        .set_width(70)
        .set_tab_expansion()
        .show_positional_help()
        .add_options()
        ("h,help", "Print usage")
        ("number-of-particles", "Number of particles (points) in a simulation", cxxopts::value<int>())
        ("seed", "Seed used for rng", cxxopts::value<double>())
        ("timesteps", "This many timesteps will be simulated", cxxopts::value<int>())
        ("disable-draw", "Disables terminal drawing", cxxopts::value<bool>()->default_value("0"))
        ("initial-position-distribution", distribution_help_text, cxxopts::value<std::string>())
        ("initial-velocity-distribution", distribution_help_text, cxxopts::value<std::string>())
        ("initial-mass-distribution", distribution_help_text, cxxopts::value<std::string>())
        ("initial-position-standard-deviation-in-meters", "Initial position standard deviation in meters", cxxopts::value<double>())
        ("initial-position-standard-deviation-in-kilometers", "Initial position standard deviation in kilometers", cxxopts::value<double>())
        ("initial-velocity-standard-deviation-in-meters-per-second", "Initial velocity standard deviation in meters per second", cxxopts::value<double>())
        ("initial-mass-standard-deviation-in-kilograms", "Initial mass standard deviation in kilograms", cxxopts::value<double>())
        ("initial-mass-mean-in-kilograms", "Initial mass mean in kilograms", cxxopts::value<double>())
        ("simulation", fmt::format("Available simulations: {}", fmt::join(simulation_names_for_help, ", ")), cxxopts::value<std::string>()->default_value(""))
        ("runtype", "Start a simulation with spesific runtype. These have unique meaning for each simulation. To see available runtypes for simulation (eg. cpu_1) use list", cxxopts::value<std::string>()->default_value(""))
        ;
        // clang-format on

        options.parse_positional({ "simulation", "runtype" });

        const auto result = options.parse(argc, argv);

        auto parse_optional_option = [&]<typename T>(const std::string option, const T) {
            return result[option].count() ? std::optional<T> { result[option].as<T>() } : std::optional<T> {};
        };

        const auto number_of_particles = parse_optional_option("number-of-particles", int {});
        const auto seed = parse_optional_option("seed", double {});
        const auto timesteps = parse_optional_option("timesteps", int {});
        /*
            Distributions:
         */
        auto parse_distribution_option = [&](const std::string& option) {
            auto distribution = std::optional<pasimulations::Distributions> {};

            if (result[option].count()) {
                switch (pasimulations::tools::hash(result[option].as<std::string>())) {
                case pasimulations::tools::hash("normal"): {
                    distribution = std::make_optional(pasimulations::Distributions::normal_distribution);
                }
                }
            }
            return distribution;
        };

        const auto initial_position_distribution = parse_distribution_option("initial-position-distribution");
        const auto initial_velocity_distribution = parse_distribution_option("initial-velocity-distribution");
        const auto initial_mass_distribution = parse_distribution_option("initial-mass-distribution");

        /*
            Parameters with units:

            Give list of option name and unit (enum classes) pairs that define the same thing but with different units
           to parsers
         */
        enum class LengthUnits { meter, kilometer };
        using option_length_unit_pair = std::pair<std::string, LengthUnits>;
        const auto parse_option_length_unit_pairs =
            [&](const std::initializer_list<option_length_unit_pair>&& option_unit_pairs) {
                using namespace units;
                using namespace units::isq;

                auto found_one = false;
                auto value = si::length<si::metre> {};

                for (const auto& [option, unit] : option_unit_pairs) {
                    if (result[option].count()) {
                        if (found_one) {
                            throw std::invalid_argument(fmt::format("Value of {} defined at least twiced!", option));
                        } else {
                            found_one = true;

                            switch (unit) {
                            case LengthUnits::meter: {
                                value = si::length<si::metre> { result[option].as<double>() };
                                break;
                            }
                            case LengthUnits::kilometer: {
                                value = si::length<si::kilometre> { result[option].as<double>() };
                                break;
                            }
                            }
                        }
                    }
                }
                auto option_value = std::optional<decltype(value)> {};
                if (found_one) {
                    option_value = std::make_optional(value);
                }
                return option_value;
            };

        const auto initial_position_standard_deviation = parse_option_length_unit_pairs(
            { { "initial-position-standard-deviation-in-meters", LengthUnits::meter },
              { "initial-position-standard-deviation-in-kilometers", LengthUnits::kilometer } });

        enum class SpeedUnits { meter_per_second };
        using option_speed_unit_pair = std::pair<std::string, SpeedUnits>;
        const auto parse_option_speed_unit_pairs =
            [&](const std::initializer_list<option_speed_unit_pair>&& option_unit_pairs) {
                using namespace units;
                using namespace units::isq;

                auto found_one = false;
                auto value = si::speed<si::metre_per_second> {};

                for (const auto& [option, unit] : option_unit_pairs) {
                    if (result[option].count()) {
                        if (found_one) {
                            throw std::invalid_argument(fmt::format("Value of {} defined at least twiced!", option));
                        } else {
                            found_one = true;

                            switch (unit) {
                            case SpeedUnits::meter_per_second: {
                                value = si::speed<si::metre_per_second> { result[option].as<double>() };
                                break;
                            }
                            }
                        }
                    }
                }
                auto option_value = std::optional<decltype(value)> {};
                if (found_one) {
                    option_value = std::make_optional(value);
                }
                return option_value;
            };

        const auto initial_velocity_standard_deviation = parse_option_speed_unit_pairs(
            { { "initial-velocity-standard-deviation-in-meters-per-second", SpeedUnits::meter_per_second } });

        enum class MassUnits { kilogram };
        using option_mass_unit_pair = std::pair<std::string, MassUnits>;
        const auto parse_option_mass_unit_pairs =
            [&](const std::initializer_list<option_mass_unit_pair>&& option_unit_pairs) {
                using namespace units;
                using namespace units::isq;

                auto found_one = false;
                auto value = si::mass<si::kilogram> {};

                for (const auto& [option, unit] : option_unit_pairs) {
                    if (result[option].count()) {
                        if (found_one) {
                            throw std::invalid_argument(fmt::format("Value of {} defined at least twiced!", option));
                        } else {
                            found_one = true;

                            switch (unit) {
                            case MassUnits::kilogram: {
                                value = si::mass<si::kilogram> { result[option].as<double>() };
                                break;
                            }
                            }
                        }
                    }
                }
                auto option_value = std::optional<decltype(value)> {};
                if (found_one) {
                    option_value = std::make_optional(value);
                }
                return option_value;
            };

        const auto initial_mass_standard_deviation =
            parse_option_mass_unit_pairs({ { "initial-mass-standard-deviation-in-kilograms", MassUnits::kilogram } });

        const auto initial_mass_mean =
            parse_option_mass_unit_pairs({ { "initial-mass-mean-in-kilograms", MassUnits::kilogram } });
        /* Boolean options: */
        const auto disable_draw = result["disable-draw"].as<bool>();

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
                auto implementation = std::optional<pasimulations::NewtonPointSimulationImplementations> {};

                switch (pasimulations::tools::hash(result["runtype"].as<std::string>())) {
                case pasimulations::tools::hash("cpu_1"): {
                    implementation = std::make_optional(pasimulations::NewtonPointSimulationImplementations::cpu_1);
                } break;
                case pasimulations::tools::hash("gpu_1"): {
                    implementation = std::make_optional(pasimulations::NewtonPointSimulationImplementations::gpu_1);
                } break;
                case pasimulations::tools::hash("gpu_2"): {
                    implementation = std::make_optional(pasimulations::NewtonPointSimulationImplementations::gpu_2);
                } break;
                case pasimulations::tools::hash("gpu_3"): {
                    implementation = std::make_optional(pasimulations::NewtonPointSimulationImplementations::gpu_3);
                } break;
                case pasimulations::tools::hash("gpu_4"): {
                    implementation = std::make_optional(pasimulations::NewtonPointSimulationImplementations::gpu_4);
                } break;
                }

                if (implementation.has_value()) {
                    pasimulations::run_newton_point_simulation_test(implementation.value(), number_of_particles, seed,
                                                                    timesteps, disable_draw);
                } else {
                    print_not_implemented_error("runtype", { "cpu_1", "gpu_1", "gpu_2", "gpu_3" });
                }
            } break;
            case pasimulations::tools::hash("newton_point_simulation_2"): {

                /* Choose integrator (positional argument) */
                auto integrator = std::optional<pasimulations::NewtonPointSimulation2Integrators> {};
                switch (pasimulations::tools::hash(result["runtype"].as<std::string>())) {
                case pasimulations::tools::hash("euler"): {
                    integrator = std::make_optional(pasimulations::NewtonPointSimulation2Integrators::euler);
                } break;
                }

                if (integrator.has_value()) {
                    pasimulations::run_newton_point_simulation_2(
                        integrator.value(), initial_position_distribution, initial_velocity_distribution,
                        initial_mass_distribution, initial_position_standard_deviation,
                        initial_velocity_standard_deviation, initial_mass_standard_deviation, initial_mass_mean,
                        number_of_particles, seed);
                } else {
                    print_not_implemented_error("runtype", { "euler" });
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
