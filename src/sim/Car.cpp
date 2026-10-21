#include "sim/Car.hpp"

#include "sim/Mesh.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <ranges>

namespace sph::sim {
namespace {

bool InBox(glm::vec3 p, glm::vec3 min, glm::vec3 max) {
  return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z &&
         p.z <= max.z;
}

bool InCylinderZ(glm::vec3 p, glm::vec3 center, float radius, float halfZ) {
  if (std::abs(p.z - center.z) > halfZ) {
    return false;
  }
  const float dx = p.x - center.x;
  const float dy = p.y - center.y;
  return dx * dx + dy * dy <= radius * radius;
}

}  // namespace

bool InsideFormulaCar(glm::vec3 point) {
  if (InBox(point, {-0.62F, 0.02F, -0.22F}, {-0.46F, 0.07F, 0.22F})) {
    return true;
  }
  if (InBox(point, {-0.50F, 0.03F, -0.07F}, {-0.22F, 0.10F, 0.07F})) {
    return true;
  }
  if (InBox(point, {-0.24F, 0.04F, -0.16F}, {0.38F, 0.13F, 0.16F})) {
    return true;
  }
  if (InBox(point, {-0.10F, 0.12F, -0.09F}, {0.16F, 0.24F, 0.09F})) {
    return true;
  }
  if (InBox(point, {0.36F, 0.14F, -0.19F}, {0.50F, 0.28F, 0.19F})) {
    return true;
  }
  if (InBox(point, {0.38F, 0.04F, -0.08F}, {0.56F, 0.12F, 0.08F})) {
    return true;
  }
  constexpr std::array Wheels{glm::vec3{-0.38F, 0.055F, -0.20F}, glm::vec3{-0.38F, 0.055F, 0.20F},
                              glm::vec3{0.32F, 0.055F, -0.20F}, glm::vec3{0.32F, 0.055F, 0.20F}};
  return std::ranges::any_of(
      Wheels, [&](glm::vec3 wheel) { return InCylinderZ(point, wheel, 0.055F, 0.045F); });
}

std::vector<MeshVertex> BuildFormulaCarMesh() { return LoadDefaultFormulaCar(CarDomain{}); }

std::vector<std::uint32_t> BuildWindOccupancy(const CarDomain& domain) {
  const std::uint32_t count = domain.grid.x * domain.grid.y * domain.grid.z;
  std::vector<std::uint32_t> occupancy(count, static_cast<std::uint32_t>(WindCell::Fluid));
  for (std::uint32_t z = 0; z < domain.grid.z; ++z) {
    for (std::uint32_t y = 0; y < domain.grid.y; ++y) {
      for (std::uint32_t x = 0; x < domain.grid.x; ++x) {
        const std::uint32_t index = x + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
        const glm::vec3 point = domain.origin + domain.cellSize * (glm::vec3(x, y, z) + 0.5F);
        if (x == 0) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Inlet);
        } else if (x + 1 == domain.grid.x) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Outlet);
        } else if (y == 0 || y + 1 == domain.grid.y || z == 0 || z + 1 == domain.grid.z) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Wall);
        }
        if (InsideFormulaCar(point)) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Solid);
        }
      }
    }
  }
  return occupancy;
}

}  // namespace sph::sim
