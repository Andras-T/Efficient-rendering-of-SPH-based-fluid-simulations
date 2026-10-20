#pragma once

#include "sim/SimParams.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string_view>

namespace sph::sim {

inline constexpr std::uint32_t TankCylinderSegments = 48;
inline constexpr std::uint32_t TankHexagonSegments = 6;

[[nodiscard]] const char* TankShapeName(TankShape shape);
[[nodiscard]] TankShape ParseTankShape(std::string_view name);

[[nodiscard]] glm::vec2 TankCenterXz(const SimParams& params);
[[nodiscard]] float TankRadius(const SimParams& params);
[[nodiscard]] float TankApothem(const SimParams& params);
[[nodiscard]] float HexagonNorm(glm::vec2 delta);
[[nodiscard]] bool InsideTank(glm::vec3 position, const SimParams& params, float inset);
void CollideTank(glm::vec3& position, glm::vec3& velocity, const SimParams& params, float radius);
void ClampAabbToTank(glm::vec3& center, glm::vec3 half, const SimParams& params);

[[nodiscard]] constexpr std::uint32_t TankMeshVertexCount(TankShape shape) {
  switch (shape) {
    case TankShape::Cylinder:
      return 12U * TankCylinderSegments;
    case TankShape::Hexagon:
      return 12U * TankHexagonSegments;
    default:
      return 36U;
  }
}

}  // namespace sph::sim
