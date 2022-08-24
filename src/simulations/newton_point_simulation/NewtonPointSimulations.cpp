#include <simulations/newton_point_simulation/NewtonPointSimulation.hpp>
#include <vulkan/vulkan.hpp>

namespace pasimulations {
namespace nps {

template <>
void NewtonPointSimulation<float>::evolve_with_gpu_1() {

    const auto desired_extensions =
        std::vector<std::string> { "GL_KHR_shader_subgroup_basic", "GL_ARB_gpu_shader_fp64" };
    kp::Manager mgr(0, {}, desired_extensions);

    auto tensor_in_x_coordinates = mgr.tensorT<float>(x_coordinates_);
    auto tensor_in_y_coordinates = mgr.tensorT<float>(y_coordinates_);
    auto tensor_in_x_speeds = mgr.tensorT<float>(x_speeds_);
    auto tensor_in_y_speeds = mgr.tensorT<float>(y_speeds_);
    auto tensor_in_masses = mgr.tensorT<float>(masses_);

    const auto parameters_for_speed_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds, tensor_in_masses };

    static const auto path_to_speed_shader =
        std::filesystem::path("./shaders/newton_point_simulation/float/gpu_1.comp.spv");

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    auto calculate_speeds =
        mgr.algorithm<float, float>(parameters_for_speed_shader, pasimulations::tools::readShader(path_to_speed_shader),
                                    kp::Workgroup { particles, 1, 1 }, { softening_radius_, G_ }, { timestep_ });

    const auto parameters_for_position_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds };

    const auto path_to_position_shader =
        std::filesystem::path("./shaders/newton_point_simulation/float/calculate_new_positions_1.comp.spv");

    auto calculate_new_positions = mgr.algorithm<float, float>(
        parameters_for_position_shader, pasimulations::tools::readShader(path_to_position_shader),
        kp::Workgroup { particles, 1, 1 }, {}, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(parameters_for_speed_shader)
        ->record<kp::OpAlgoDispatch>(calculate_speeds, std::vector<float> { timestep_ })
        ->record<kp::OpAlgoDispatch>(calculate_new_positions, std::vector<float> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(parameters_for_speed_shader)
        ->eval();

    simulation_time_ += timestep_;

    x_coordinates_ = tensor_in_x_coordinates->vector();
    y_coordinates_ = tensor_in_y_coordinates->vector();
    x_speeds_ = tensor_in_x_speeds->vector();
    y_speeds_ = tensor_in_y_speeds->vector();
}

template <>
void NewtonPointSimulation<float>::evolve_with_gpu_2() {

    const auto desired_extensions =
        std::vector<std::string> { "GL_KHR_shader_subgroup_basic", "GL_ARB_gpu_shader_fp64" };
    kp::Manager mgr(0, {}, desired_extensions);

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    /* This needs to be kept up to date with gpu_2.comp */
    constexpr auto size_of_workgroup = uint32_t { 64 };
    const auto workgroups = particles / size_of_workgroup + 1;
    const auto size_of_padding = particles % size_of_workgroup;

    auto x_coordinates_with_padding = std::move(x_coordinates_);
    auto y_coordinates_with_padding = std::move(y_coordinates_);
    auto x_speeds_with_padding = std::move(x_speeds_);
    auto y_speeds_with_padding = std::move(y_speeds_);
    auto masses_with_padding = masses_;

    x_coordinates_with_padding.resize(particles + size_of_padding, 0.0);
    y_coordinates_with_padding.resize(particles + size_of_padding, 0.0);
    x_speeds_with_padding.resize(particles + size_of_padding, 0.0);
    y_speeds_with_padding.resize(particles + size_of_padding, 0.0);
    /* Padded particles have importantly masses of zero so they do not have effect */
    masses_with_padding.resize(particles + size_of_padding, 0.0);

    auto tensor_in_x_coordinates = mgr.tensorT<float>(x_coordinates_with_padding);
    auto tensor_in_y_coordinates = mgr.tensorT<float>(y_coordinates_with_padding);
    auto tensor_in_x_speeds = mgr.tensorT<float>(x_speeds_with_padding);
    auto tensor_in_y_speeds = mgr.tensorT<float>(y_speeds_with_padding);
    auto tensor_in_masses = mgr.tensorT<float>(masses_with_padding);

    const auto parameters_for_speed_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds, tensor_in_masses };

    const auto path_to_speed_shader = std::filesystem::path("./shaders/newton_point_simulation/float/gpu_2.comp.spv");

    auto calculate_speeds = mgr.algorithm<uint32_t, float>(
        parameters_for_speed_shader, pasimulations::tools::readShader(path_to_speed_shader),
        kp::Workgroup { workgroups, 1, 1 }, { particles }, { softening_radius_, G_, timestep_ });

    const auto parameters_for_position_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds };

    const auto path_to_position_shader =
        std::filesystem::path("./shaders/newton_point_simulation/float/calculate_new_positions_2.comp.spv");

    auto calculate_new_positions = mgr.algorithm<uint32_t, float>(
        parameters_for_position_shader, pasimulations::tools::readShader(path_to_position_shader),
        kp::Workgroup { workgroups, 1, 1 }, {}, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(parameters_for_speed_shader)
        ->record<kp::OpAlgoDispatch>(calculate_speeds, std::vector<float> { softening_radius_, G_, timestep_ })
        ->record<kp::OpAlgoDispatch>(calculate_new_positions, std::vector<float> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(parameters_for_position_shader)
        ->eval();

    simulation_time_ += timestep_;

    x_coordinates_ = tensor_in_x_coordinates->vector();
    y_coordinates_ = tensor_in_y_coordinates->vector();
    x_speeds_ = tensor_in_x_speeds->vector();
    y_speeds_ = tensor_in_y_speeds->vector();

    x_coordinates_.resize(particles);
    y_coordinates_.resize(particles);
    x_speeds_.resize(particles);
    y_speeds_.resize(particles);
}

template <>
void NewtonPointSimulation<float>::evolve_with_gpu_3() {

    const auto desired_extensions =
        std::vector<std::string> { "GL_KHR_shader_subgroup_basic", "GL_ARB_gpu_shader_fp64" };
    kp::Manager mgr(0, {}, desired_extensions);

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    /* This needs to be kept up to date with gpu_2.comp */
    constexpr auto size_of_workgroup = uint32_t { 64 };
    const auto workgroups = particles / size_of_workgroup + 1;
    const auto size_of_padding = particles % size_of_workgroup;

    auto x_coordinates_with_padding = std::move(x_coordinates_);
    auto y_coordinates_with_padding = std::move(y_coordinates_);
    auto x_speeds_with_padding = std::move(x_speeds_);
    auto y_speeds_with_padding = std::move(y_speeds_);
    auto masses_with_padding = masses_;

    x_coordinates_with_padding.resize(particles + size_of_padding, 0.0);
    y_coordinates_with_padding.resize(particles + size_of_padding, 0.0);
    x_speeds_with_padding.resize(particles + size_of_padding, 0.0);
    y_speeds_with_padding.resize(particles + size_of_padding, 0.0);
    /* Padded particles have importantly masses of zero so they do not have effect */
    masses_with_padding.resize(particles + size_of_padding, 0.0);

    auto tensor_in_x_coordinates = mgr.tensorT<float>(x_coordinates_with_padding);
    auto tensor_in_y_coordinates = mgr.tensorT<float>(y_coordinates_with_padding);
    auto tensor_in_x_speeds = mgr.tensorT<float>(x_speeds_with_padding);
    auto tensor_in_y_speeds = mgr.tensorT<float>(y_speeds_with_padding);
    auto tensor_in_masses = mgr.tensorT<float>(masses_with_padding);

    const auto parameters_for_speed_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds, tensor_in_masses };

    const auto path_to_speed_shader = std::filesystem::path("./shaders/newton_point_simulation/float/gpu_3.comp.spv");

    auto calculate_speeds = mgr.algorithm<uint32_t, float>(
        parameters_for_speed_shader, pasimulations::tools::readShader(path_to_speed_shader),
        kp::Workgroup { workgroups, 1, 1 }, { particles }, { softening_radius_, G_, timestep_ });

    const auto parameters_for_position_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds };

    const auto path_to_position_shader =
        std::filesystem::path("./shaders/newton_point_simulation/float/calculate_new_positions_2.comp.spv");

    auto calculate_new_positions = mgr.algorithm<uint32_t, float>(
        parameters_for_position_shader, pasimulations::tools::readShader(path_to_position_shader),
        kp::Workgroup { workgroups, 1, 1 }, {}, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(parameters_for_speed_shader)
        ->record<kp::OpAlgoDispatch>(calculate_speeds, std::vector<float> { softening_radius_, G_, timestep_ })
        ->record<kp::OpAlgoDispatch>(calculate_new_positions, std::vector<float> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(parameters_for_position_shader)
        ->eval();

    simulation_time_ += timestep_;

    x_coordinates_ = tensor_in_x_coordinates->vector();
    y_coordinates_ = tensor_in_y_coordinates->vector();
    x_speeds_ = tensor_in_x_speeds->vector();
    y_speeds_ = tensor_in_y_speeds->vector();

    x_coordinates_.resize(particles);
    y_coordinates_.resize(particles);
    x_speeds_.resize(particles);
    y_speeds_.resize(particles);
}

template <>
void NewtonPointSimulation<float>::evolve_with_gpu_4() {

    const auto desired_extensions =
        std::vector<std::string> { "GL_KHR_shader_subgroup_basic", "GL_ARB_gpu_shader_fp64" };
    kp::Manager mgr(0, {}, desired_extensions);


    fmt::print("{}", pasimulations::tools::subgroup_size(mgr));

    const auto particles = static_cast<uint32_t>(x_coordinates_.size());
    /* This needs to be kept up to date with gpu_2.comp */
    constexpr auto size_of_workgroup = uint32_t { 64 };
    const auto workgroups = particles / size_of_workgroup + 1;
    const auto size_of_padding = particles % size_of_workgroup;

    auto x_coordinates_with_padding = std::move(x_coordinates_);
    auto y_coordinates_with_padding = std::move(y_coordinates_);
    auto x_speeds_with_padding = std::move(x_speeds_);
    auto y_speeds_with_padding = std::move(y_speeds_);
    auto masses_with_padding = masses_;

    x_coordinates_with_padding.resize(particles + size_of_padding, 0.0);
    y_coordinates_with_padding.resize(particles + size_of_padding, 0.0);
    x_speeds_with_padding.resize(particles + size_of_padding, 0.0);
    y_speeds_with_padding.resize(particles + size_of_padding, 0.0);
    /* Padded particles have importantly masses of zero so they do not have effect */
    masses_with_padding.resize(particles + size_of_padding, 0.0);

    auto tensor_in_x_coordinates = mgr.tensor(x_coordinates_with_padding);
    auto tensor_in_y_coordinates = mgr.tensor(y_coordinates_with_padding);
    auto tensor_in_x_speeds = mgr.tensor(x_speeds_with_padding);
    auto tensor_in_y_speeds = mgr.tensor(y_speeds_with_padding);
    auto tensor_in_masses = mgr.tensor(masses_with_padding);

    const auto parameters_for_speed_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds, tensor_in_masses };

    const auto path_to_speed_shader = std::filesystem::path("./shaders/newton_point_simulation/float/gpu_3.comp.spv");

    auto calculate_speeds = mgr.algorithm<uint32_t, float>(
        parameters_for_speed_shader, pasimulations::tools::readShader(path_to_speed_shader),
        kp::Workgroup { workgroups, 1, 1 }, { particles }, { softening_radius_, G_, timestep_ });

    const auto parameters_for_position_shader =
        std::vector<std::shared_ptr<kp::Tensor>> { tensor_in_x_coordinates, tensor_in_y_coordinates, tensor_in_x_speeds,
                                                   tensor_in_y_speeds };

    const auto path_to_position_shader =
        std::filesystem::path("./shaders/newton_point_simulation/float/calculate_new_positions_2.comp.spv");

    auto calculate_new_positions = mgr.algorithm<uint32_t, float>(
        parameters_for_position_shader, pasimulations::tools::readShader(path_to_position_shader),
        kp::Workgroup { workgroups, 1, 1 }, {}, { timestep_ });

    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(parameters_for_speed_shader)
        ->record<kp::OpAlgoDispatch>(calculate_speeds, std::vector<float> { softening_radius_, G_, timestep_ })
        ->record<kp::OpAlgoDispatch>(calculate_new_positions, std::vector<float> { timestep_ })
        ->record<kp::OpTensorSyncLocal>(parameters_for_position_shader)
        ->eval();

    simulation_time_ += timestep_;

    x_coordinates_ = tensor_in_x_coordinates->vector();
    y_coordinates_ = tensor_in_y_coordinates->vector();
    x_speeds_ = tensor_in_x_speeds->vector();
    y_speeds_ = tensor_in_y_speeds->vector();

    x_coordinates_.resize(particles);
    y_coordinates_.resize(particles);
    x_speeds_.resize(particles);
    y_speeds_.resize(particles);
}

} // namespace nps
} // namespace pasimulations