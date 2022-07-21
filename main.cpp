#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define KOMPUTE_LOG_LEVEL 5
#include "../libs/kompute/single_include/kompute/Kompute.hpp"
#pragma GCC diagnostic pop

// This is just to suppress warings 
#include ".vscode/suppress_intellisense_warnings.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "NewtonPointSimulation.hpp"

// Forward declaring our helper function to read compiled shader
static std::vector<uint32_t> readShader(const std::string shader_path);



int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    auto simulator = NPS::NewtonPointSimulation<units::isq::si::metre, units::isq::si::kilogram, units::isq::si::second>{};

}

[[maybe_unused]] static std::vector<uint32_t> readShader(const std::string shader_path) {
    std::ifstream fileStream(shader_path, std::ios::binary);
    std::vector<char> buffer;
    buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
    return { (uint32_t*)buffer.data(), (uint32_t*)(buffer.data() + buffer.size()) };
}
