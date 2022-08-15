#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define KOMPUTE_LOG_LEVEL 5
#include "../libs/kompute/single_include/kompute/Kompute.hpp"
#pragma GCC diagnostic pop

#include <units/isq/si/acceleration.h>
#include <units/isq/si/constants.h>
#include <units/isq/si/force.h>
#include <units/isq/si/length.h>
#include <units/isq/si/mass.h>
#include <units/isq/si/time.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <ANSI.hpp>

namespace pasimulations {

namespace nps {
using namespace units;
using namespace units::isq;

template <>
void NewtonPointSimulation<units::isq::si::metre, units::isq::si::kilogram, units::isq::si::second,
                           units::isq::si::metre_per_second, units::isq::si::metre_per_second_sq>::evolve_with_gpu_1() {
    kp::Manager mgr {};

    auto tensor_in_a = mgr.tensorT(std::vector<float> { 1, 2, 3 });
    auto tensor_in_b = mgr.tensorT(std::vector<float> { 1, 2, 3 });

    const std::vector<std::shared_ptr<kp::Tensor>> params { tensor_in_a, tensor_in_b };
    fmt::print("moi");
    std::shared_ptr<kp::Algorithm> algo =
        mgr.algorithm(params, pasimulations::tools::readShader("testing.comp.spv"), { 1, 1, 1 });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(params)
        ->record<kp::OpAlgoDispatch>(algo)
        ->record<kp::OpTensorSyncLocal>(params)
        ->eval();

    fmt::print("{}", fmt::join(tensor_in_a->vector(), " "));
}

} // namespace nps
} // namespace pasimulations