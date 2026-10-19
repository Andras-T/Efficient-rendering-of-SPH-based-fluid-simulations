#include "sim/SimParams.hpp"

#include <format>
#include <stdexcept>

namespace sph::sim {

const char* SolverMethodName(SolverMethod method) {
  switch (method) {
    case SolverMethod::Dfsph:
      return "dfsph";
    default:
      return "wcsph";
  }
}

SolverMethod ParseSolverMethod(std::string_view name) {
  if (name == "wcsph" || name == "wc") {
    return SolverMethod::Wcsph;
  }
  if (name == "dfsph" || name == "df") {
    return SolverMethod::Dfsph;
  }
  throw std::invalid_argument(std::format("unknown solver '{}'", name));
}

float SimParams::mass() const {
  return restDensity * particleSpacing * particleSpacing * particleSpacing;
}

float SimParams::supportRadius() const { return particleSpacing * 2.0F; }

float SimParams::renderRadius() const { return particleSpacing * 0.72F; }

float SimParams::stiffness() const { return restDensity * speedOfSound * speedOfSound / eosGamma; }

}  // namespace sph::sim
