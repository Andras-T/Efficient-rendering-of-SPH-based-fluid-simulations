#include "sim/SimParams.hpp"

namespace sph::sim {

float SimParams::mass() const {
  return restDensity * particleSpacing * particleSpacing * particleSpacing;
}

float SimParams::supportRadius() const { return particleSpacing * 2.0F; }

float SimParams::renderRadius() const { return particleSpacing * 0.72F; }

float SimParams::stiffness() const { return restDensity * speedOfSound * speedOfSound / eosGamma; }

}  // namespace sph::sim
