#pragma once

#include "sim/Particle.hpp"

#include <glm/vec3.hpp>

#include <cstdint>

namespace sph::sim {

struct SimParams {
  float particleSpacing = 0.04F;
  float restDensity = 1000.0F;
  glm::vec3 domainMin{-0.70F, 0.00F, -0.40F};
  glm::vec3 domainMax{0.70F, 1.00F, 0.40F};
  glm::vec3 gravity{0.0F, -9.81F, 0.0F};
  float viscosity = 0.35F;
  float speedOfSound = 24.0F;
  float eosGamma = 7.0F;
  float restitution = 0.05F;
  float timeStep = 0.0012F;
  std::uint32_t substeps = 8;
  SolverMethod solver = SolverMethod::Wcsph;
  std::uint32_t densityIterations = 2;

  [[nodiscard]] float mass() const;
  [[nodiscard]] float supportRadius() const;
  [[nodiscard]] float renderRadius() const;
  [[nodiscard]] float stiffness() const;
};

}  // namespace sph::sim
