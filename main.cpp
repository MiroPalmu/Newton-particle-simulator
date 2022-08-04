#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "pasimulations.hpp"

#define FMT_HEADER_ONLY
#include "ANSI.hpp"
#include "fmt/format.h"
#include "pasimulations.hpp"
#include "tools.hpp"
#include <cxxopts.hpp>

// Forward declaring our helper function to read compiled shader
static std::vector<uint32_t> readShader(const std::string shader_path);

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
        ("h,help", "Print usage")
        ("number-of-particles", "Number of particles (points) in a simulation", cxxopts::value<int>()->default_value("1500")->implicit_value("1500"))
        ("simulation", fmt::format("Available simulations: {}", fmt::join(simulation_names_for_help, ", ")), cxxopts::value<std::string>()->default_value(""))
        ("runtype", "Start a simulation with spesific runtype. To see available runtypes (eg. cpu_1) use list", cxxopts::value<std::string>()->default_value(""))
        ;
        // clang-format on

        options.parse_positional({ "simulation", "runtype" });

        const auto result = options.parse(argc, argv);

        const auto error_font = ansi::str(ansi::fg_brightred) + ansi::str(ansi::bold);

        if (result.count("help") || !(result["simulation"].count() && result["runtype"].count())) {
            fmt::print(options.help());
        } else {
            switch (pasimulations::tools::hash(result["simulation"].as<std::string>())) {
            case pasimulations::tools::hash("newton_point_simulation"):

                switch (pasimulations::tools::hash(result["runtype"].as<std::string>())) {
                case pasimulations::tools::hash("cpu_1"):
                    pasimulations::run_newton_point_simulation_test(
                        result["number-of-particles"].as<int>(),
                        pasimulations::Newton_point_simulation_implementations::cpu_1);
                    break;
                case pasimulations::tools::hash("gpu_1"):
                    pasimulations::run_newton_point_simulation_test(
                        result["number-of-particles"].as<int>(),
                        pasimulations::Newton_point_simulation_implementations::gpu_1);
                    break;

                default:
                    fmt::print("{}{}{} is not implemented\n", error_font, result["runtype"].as<std::string>(),
                               ansi::str(ansi::reset));
                    fmt::print("Following runtypes available:\n");
                    /*
                                        THIS HAS TO BE UPDATED MANUALLY:
                     */
                    const auto runtypes_for_nps = std::vector<std::string> { "cpu_1", "gpu_1" }; // Can be made consexpr in gcc11
                    for (const auto& runtype : runtypes_for_nps) {
                        fmt::print("\t{}\n", runtype);
                    }
                    break;
                }

                break;
            default:
                fmt::print("{}{}{} is not implemented\n", error_font, result["simulation"].as<std::string>(),
                           ansi::str(ansi::reset));
                fmt::print("Following simulations available:\n\t{}\n", fmt::join(simulation_names_for_help, "\n\t"));
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

[[maybe_unused]] static std::vector<uint32_t> readShader(const std::string shader_path) {
    std::ifstream fileStream(shader_path, std::ios::binary);
    std::vector<char> buffer;
    buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
    return { (uint32_t*)buffer.data(), (uint32_t*)(buffer.data() + buffer.size()) };
}
