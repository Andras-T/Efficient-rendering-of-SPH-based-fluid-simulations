#pragma once

#include "sim/Car.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <span>

namespace sph::sim {

struct WindProbe {
  bool hit = false;
  bool onBody = false;
  bool onGround = false;
  glm::vec3 position{0.0F};
  glm::vec3 velocity{0.0F};
  float density = 1.0F;
  float pressure = 1.0F / 3.0F;
  float cp = 0.0F;
  float speedLattice = 0.0F;
  float speedMs = 0.0F;
  float speedKmh = 0.0F;
};

[[nodiscard]] WindProbe SampleWindPoint(const CarDomain& domain, std::span<const glm::vec4> macros,
                                        std::span<const std::uint32_t> occupancy, float inletUx,
                                        glm::vec3 point);

// March `origin + t * direction` through occupancy. A solid cell is an on-body hit; the y=0 wall
// is the floor. Ceiling and side enclosure walls are skipped. Otherwise the sample sits 32% of
// the way through the tunnel AABB.
[[nodiscard]] WindProbe ProbeWindRay(const CarDomain& domain, std::span<const glm::vec4> macros,
                                     std::span<const std::uint32_t> occupancy, float inletUx,
                                     glm::vec3 origin, glm::vec3 direction);

}  // namespace sph::sim
