#include "sim/Car.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <ranges>

namespace sph::sim {
namespace {

constexpr glm::vec3 BodyRed{0.78F, 0.07F, 0.10F};
constexpr glm::vec3 WingRed{0.62F, 0.05F, 0.08F};
constexpr glm::vec3 Carbon{0.08F, 0.08F, 0.09F};
constexpr glm::vec3 Rubber{0.06F, 0.06F, 0.07F};
constexpr glm::vec3 Halo{0.12F, 0.12F, 0.13F};

void AddTriangle(std::vector<MeshVertex>& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                 glm::vec3 color) {
  glm::vec3 normal = glm::cross(b - a, c - a);
  const float length = glm::length(normal);
  if (length < 1.0e-8F) {
    return;
  }
  normal /= length;
  mesh.push_back({.position = a, .normal = normal, .color = color});
  mesh.push_back({.position = b, .normal = normal, .color = color});
  mesh.push_back({.position = c, .normal = normal, .color = color});
}

void AddBox(std::vector<MeshVertex>& mesh, glm::vec3 min, glm::vec3 max, glm::vec3 color) {
  const std::array p{
      glm::vec3{min.x, min.y, min.z}, glm::vec3{max.x, min.y, min.z},
      glm::vec3{max.x, max.y, min.z}, glm::vec3{min.x, max.y, min.z},
      glm::vec3{min.x, min.y, max.z}, glm::vec3{max.x, min.y, max.z},
      glm::vec3{max.x, max.y, max.z}, glm::vec3{min.x, max.y, max.z},
  };
  const std::array faces{std::array{1, 5, 6, 2}, std::array{0, 3, 7, 4}, std::array{3, 2, 6, 7},
                         std::array{0, 4, 5, 1}, std::array{4, 7, 6, 5}, std::array{1, 2, 3, 0}};
  for (const auto& face : faces) {
    AddTriangle(mesh, p.at(static_cast<std::size_t>(face[0])),
                p.at(static_cast<std::size_t>(face[1])), p.at(static_cast<std::size_t>(face[2])),
                color);
    AddTriangle(mesh, p.at(static_cast<std::size_t>(face[0])),
                p.at(static_cast<std::size_t>(face[2])), p.at(static_cast<std::size_t>(face[3])),
                color);
  }
}

void AddCylinderZ(std::vector<MeshVertex>& mesh, glm::vec3 center, float radius, float halfZ,
                  glm::vec3 color, int segments = 14) {
  const float z0 = center.z - halfZ;
  const float z1 = center.z + halfZ;
  for (int i = 0; i < segments; ++i) {
    const float a0 = (static_cast<float>(i) / static_cast<float>(segments)) * 6.2831853F;
    const float a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * 6.2831853F;
    const glm::vec3 r0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius, 0.0F};
    const glm::vec3 r1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius, 0.0F};
    const glm::vec3 a{r0.x, r0.y, z0};
    const glm::vec3 b{r1.x, r1.y, z0};
    const glm::vec3 c{r1.x, r1.y, z1};
    const glm::vec3 d{r0.x, r0.y, z1};
    AddTriangle(mesh, a, b, c, color);
    AddTriangle(mesh, a, c, d, color);
    const glm::vec3 cap0{center.x, center.y, z0};
    const glm::vec3 cap1{center.x, center.y, z1};
    AddTriangle(mesh, cap0, b, a, color);
    AddTriangle(mesh, cap1, d, c, color);
  }
}

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

std::vector<MeshVertex> BuildFormulaCarMesh() {
  std::vector<MeshVertex> mesh;
  mesh.reserve(1800);
  AddBox(mesh, {-0.62F, 0.02F, -0.22F}, {-0.46F, 0.07F, 0.22F}, WingRed);
  AddBox(mesh, {-0.50F, 0.03F, -0.07F}, {-0.22F, 0.10F, 0.07F}, BodyRed);
  AddBox(mesh, {-0.24F, 0.04F, -0.16F}, {0.38F, 0.13F, 0.16F}, BodyRed);
  AddBox(mesh, {-0.22F, 0.06F, -0.20F}, {0.20F, 0.12F, 0.20F}, BodyRed);
  AddBox(mesh, {-0.10F, 0.12F, -0.09F}, {0.16F, 0.24F, 0.09F}, Carbon);
  AddBox(mesh, {-0.04F, 0.20F, -0.07F}, {0.10F, 0.26F, 0.07F}, Halo);
  AddBox(mesh, {0.36F, 0.14F, -0.19F}, {0.50F, 0.28F, 0.19F}, WingRed);
  AddBox(mesh, {0.38F, 0.04F, -0.08F}, {0.56F, 0.12F, 0.08F}, Carbon);
  AddBox(mesh, {0.48F, 0.12F, -0.04F}, {0.52F, 0.26F, 0.04F}, Carbon);
  AddCylinderZ(mesh, {-0.38F, 0.055F, -0.20F}, 0.055F, 0.045F, Rubber);
  AddCylinderZ(mesh, {-0.38F, 0.055F, 0.20F}, 0.055F, 0.045F, Rubber);
  AddCylinderZ(mesh, {0.32F, 0.055F, -0.20F}, 0.055F, 0.045F, Rubber);
  AddCylinderZ(mesh, {0.32F, 0.055F, 0.20F}, 0.055F, 0.045F, Rubber);
  return mesh;
}

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
