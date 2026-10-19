#pragma once

#include <glm/vec3.hpp>

namespace sph::sim {

struct SimParams {
  float particleSpacing = 0.04F;
  float restDensity = 1000.0F;
  glm::vec3 domainMin{-0.70F, 0.00F, -0.40F};
  glm::vec3 domainMax{0.70F, 1.00F, 0.40F};

  [[nodiscard]] float mass() const;
  [[nodiscard]] float supportRadius() const;
  [[nodiscard]] float renderRadius() const;
};

}  // namespace sph::sim
