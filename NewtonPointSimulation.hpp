#pragma once
#include "units.hpp"
#include <vector>
namespace NPS {

/* 
Stores data as units from template arguments.
Primitive data type is defnied in units.hpp
 */
template<UnitOf<si::dim_length> coordniate_unit, UnitOf<si::dim_mass> mass_units, UnitOf<si::dim_time> simulation_time_units>
class NewtonPointSimulation {

  private:
  std::vector<x_t<coordniate_unit>> x_coordinates;
  std::vector<y_t<coordniate_unit>> y_coordinates;
  std::vector<z_t<coordniate_unit>> z_coordinates;

  std::vector<particle_mass_t<mass_units>> particle_masses;
  
  simulation_time_t<simulation_time_units> simulation_time;
  public:
    // NPS();
    // ~NPS();
};

} // namespace NPS