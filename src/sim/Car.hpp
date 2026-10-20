#pragma once

#include <glm/ext/vector_uint3.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace sph::sim {

struct MeshVertex {
  glm::vec3 position{0.0F};
  float pad0 = 0.0F;
  glm::vec3 normal{0.0F, 1.0F, 0.0F};
  float pad1 = 0.0F;
  glm::vec3 color{1.0F};
  float pad2 = 0.0F;
};

static_assert(sizeof(MeshVertex) == 48);

struct CarDomain {
  glm::vec3 origin{-1.70F, 0.0F, -0.64F};
  float cellSize = 0.04F;
  glm::uvec3 grid{96, 24, 32};
};

[[nodiscard]] constexpr glm::vec3 CarDomainMax(const CarDomain& domain) {
  return domain.origin + domain.cellSize * glm::vec3(domain.grid);
}

[[nodiscard]] bool InsideFormulaCar(glm::vec3 point);
[[nodiscard]] std::vector<MeshVertex> BuildFormulaCarMesh();
[[nodiscard]] std::vector<std::uint32_t> BuildWindOccupancy(const CarDomain& domain);

enum class WindCell : std::uint8_t {
  Fluid = 0,
  Solid = 1,
  Inlet = 2,
  Outlet = 3,
  Wall = 4,
};

}  // namespace sph::sim
