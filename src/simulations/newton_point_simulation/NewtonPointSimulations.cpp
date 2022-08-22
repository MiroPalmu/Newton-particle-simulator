#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>

namespace pasimulations {
namespace nps {

template <>
void NewtonPointSimulation<double>::evolve_with_gpu_1() {

    const auto desired_extensions =
        std::vector<std::string> { "GL_KHR_shader_subgroup_basic", "GL_ARB_gpu_shader_fp64" };
    kp::Manager mgr(0, {}, desired_extensions);

    auto tensor_in_x_coordinates = mgr.tensorT<double>(x_coordinates_);
    auto tensor_in_y_coordinates = mgr.tensorT<double>(y_coordinates_);
    auto tensor_in_x_speeds = mgr.tensorT<double>(x_speeds_);
    auto tensor_in_y_speeds = mgr.tensorT<double>(y_speeds_);
    auto tensor_in_masses = mgr.tensorT<double>(masses_);

    const auto parameters_for_speed_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds, tensor_in_masses };

    static const auto path_to_speed_shader =
        std::filesystem::path("./shaders/newton_point_simulation/double/gpu_1.comp.spv");

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    auto calculate_speeds = mgr.algorithm<double, double>(
        parameters_for_speed_shader, pasimulations::tools::readShader(path_to_speed_shader),
        kp::Workgroup { particles, 1, 1 }, { softening_radius_, G_ }, { timestep_ });

    const auto parameters_for_position_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds };

    const auto path_to_position_shader =
        std::filesystem::path("./shaders/newton_point_simulation/double/calculate_new_positions_1.comp.spv");

    auto calculate_new_positions = mgr.algorithm<double, double>(
        parameters_for_position_shader, pasimulations::tools::readShader(path_to_position_shader),
        kp::Workgroup { particles, 1, 1 }, {}, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(parameters_for_speed_shader)
        ->record<kp::OpAlgoDispatch>(calculate_speeds, std::vector<double> { timestep_ })
        ->record<kp::OpAlgoDispatch>(calculate_new_positions, std::vector<double> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(parameters_for_speed_shader)
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