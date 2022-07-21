#pragma once

#include <units/kind.h>
#include <units/quantity.h>
#include <units/quantity_kind.h>
#include <units/quantity_point_kind.h>
#include <units/isq/si/length.h>
#include <units/isq/si/mass.h>
#include <units/isq/si/time.h>

namespace NPS {
    
    using namespace units;
    using namespace units::isq;

namespace primitive_types {
    using real = double;

}

    // Time:
    struct simulation_time_kind : kind<simulation_time_kind, si::dim_time> {};

    struct simulation_point_kind : point_kind<simulation_point_kind, simulation_time_kind> {};

    template<UnitOf<si::dim_time> T>
    using simulation_time_t = quantity_point_kind<simulation_point_kind, T, primitive_types::real>;

    // Mass:
    struct particle_mass_kind : kind<particle_mass_kind, si::dim_mass> {};

    template<UnitOf<si::dim_mass> M>
    using particle_mass_t = quantity_kind<particle_mass_kind, M, primitive_types::real>;


    // You have to have separated kind for each coordinate
    // To be able to get more readable compilation errors with down casting
    // https://mpusz.github.io/units/design/downcasting.html#the-downcasting-facility
    
    struct x_coordinate_kind : kind<x_coordinate_kind, si::dim_length> {};
    struct y_coordinate_kind : kind<y_coordinate_kind, si::dim_length> {};
    struct z_coordinate_kind : kind<z_coordinate_kind, si::dim_length> {};

    struct x_coordinate_point_kind : point_kind<x_coordinate_point_kind, x_coordinate_kind> {};
    struct y_coordinate_point_kind : point_kind<y_coordinate_point_kind, y_coordinate_kind> {};
    struct z_coordinate_point_kind : point_kind<z_coordinate_point_kind, z_coordinate_kind> {};

    template<UnitOf<si::dim_length> L>
    using x_t = quantity_point_kind<x_coordinate_point_kind, L, primitive_types::real>;

    template<UnitOf<si::dim_length> L>
    using y_t = quantity_point_kind<y_coordinate_point_kind, L, primitive_types::real>;

    template<UnitOf<si::dim_length> L>
    using z_t = quantity_point_kind<z_coordinate_point_kind, L, primitive_types::real>;
    
}