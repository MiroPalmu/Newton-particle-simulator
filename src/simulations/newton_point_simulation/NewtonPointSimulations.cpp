#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>

namespace pasimulations {
namespace nps {

template <>
void NewtonPointSimulation<double>::evolve_with_gpu_1() {

    kp::Manager mgr {};

    auto tensor_in_x_coordinates = mgr.tensorT(x_coordinates_);
    auto tensor_in_y_coordinates = mgr.tensorT(y_coordinates_);
    auto tensor_in_x_speeds = mgr.tensorT(x_speeds_);
    auto tensor_in_y_speeds = mgr.tensorT(y_speeds_);
    auto tensor_in_masses = mgr.tensorT(masses_);

    const std::vector<std::shared_ptr<kp::Tensor>> params { tensor_in_x_coordinates, tensor_in_y_coordinates,
                                                            tensor_in_x_speeds, tensor_in_y_speeds, tensor_in_masses };

    static const auto path_to_shader = std::filesystem::path("./shaders/gpu_1_double.comp.spv");

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    std::shared_ptr<kp::Algorithm> algo =
        mgr.algorithm<double, double>(params, pasimulations::tools::readShader(path_to_shader),
                                      kp::Workgroup { particles, 1, 1 }, { softening_radius_, G_ }, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(params)
        ->record<kp::OpAlgoDispatch>(algo, std::vector<double> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(params)
        ->eval();

    simulation_time_ += timestep_;

    x_coordinates_ = tensor_in_x_coordinates->vector();
    y_coordinates_ = tensor_in_y_coordinates->vector();
    x_speeds_ = tensor_in_x_speeds->vector();
    y_speeds_ = tensor_in_y_speeds->vector();
}


template <>
void NewtonPointSimulation<double>::evolve_with_gpu_2() {

    kp::Manager mgr {};

    auto tensor_in_x_coordinates = mgr.tensorT(x_coordinates_);
    auto tensor_in_y_coordinates = mgr.tensorT(y_coordinates_);
    auto tensor_in_x_speeds = mgr.tensorT(x_speeds_);
    auto tensor_in_y_speeds = mgr.tensorT(y_speeds_);
    auto tensor_in_masses = mgr.tensorT(masses_);

    const std::vector<std::shared_ptr<kp::Tensor>> params { tensor_in_x_coordinates, tensor_in_y_coordinates,
                                                            tensor_in_x_speeds, tensor_in_y_speeds, tensor_in_masses };

    static const auto path_to_shader = std::filesystem::path("./shaders/gpu_2_double.comp.spv");

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    std::shared_ptr<kp::Algorithm> algo =
        mgr.algorithm<double, double>(params, pasimulations::tools::readShader(path_to_shader),
                                      kp::Workgroup { particles, 1, 1 }, { softening_radius_, G_ }, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(params)
        ->record<kp::OpAlgoDispatch>(algo, std::vector<double> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(params)
        ->eval();

    simulation_time_ += timestep_;

    x_coordinates_ = tensor_in_x_coordinates->vector();
    y_coordinates_ = tensor_in_y_coordinates->vector();
    x_speeds_ = tensor_in_x_speeds->vector();
    y_speeds_ = tensor_in_y_speeds->vector();
}

} // namespace nps
} // namespace pasimulations