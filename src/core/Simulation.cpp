#include "core/Simulation.hpp"

#include <format>
#include <stdexcept>

namespace sph {

const char* SimulationKindName(SimulationKind kind) {
  switch (kind) {
    case SimulationKind::WindTunnel:
      return "wind";
    case SimulationKind::Smoke:
      return "smoke";
    default:
      return "fluid";
  }
}

const char* SimulationKindLabel(SimulationKind kind) {
  switch (kind) {
    case SimulationKind::WindTunnel:
      return "Wind tunnel";
    case SimulationKind::Smoke:
      return "Smoke";
    default:
      return "Fluid";
  }
}

SimulationKind ParseSimulationKind(std::string_view name) {
  if (name == "fluid" || name == "sph") {
    return SimulationKind::Fluid;
  }
  if (name == "wind" || name == "windtunnel") {
    return SimulationKind::WindTunnel;
  }
  if (name == "smoke") {
    return SimulationKind::Smoke;
  }
  throw std::invalid_argument(std::format("unknown simulation '{}'", name));
}

}  // namespace sph
