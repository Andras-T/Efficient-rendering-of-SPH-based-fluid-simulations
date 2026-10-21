#include "sim/Car.hpp"
#include "sim/Mesh.hpp"

#include <glm/common.hpp>
#include <gtest/gtest.h>

#include <cstdint>

namespace sph::sim {
namespace {

TEST(CarTest, MeshHasABody) {
  const auto mesh = BuildFormulaCarMesh();
  EXPECT_GT(mesh.size(), 20000U);
  glm::vec3 mn = mesh.front().position;
  glm::vec3 mx = mn;
  for (const MeshVertex& vertex : mesh) {
    mn = glm::min(mn, vertex.position);
    mx = glm::max(mx, vertex.position);
  }
  EXPECT_GT(mx.x - mn.x, 1.0F);
  EXPECT_GT(mx.z - mn.z, 0.45F);
  EXPECT_GT(mx.y - mn.y, 0.18F);
}

TEST(CarTest, OccupancyMarksTheCarAndInlet) {
  const CarDomain domain;
  const auto mesh = BuildFormulaCarMesh();
  const auto occupancy = BuildWindOccupancy(domain, mesh);
  EXPECT_EQ(occupancy.size(), domain.grid.x * domain.grid.y * domain.grid.z);
  std::uint32_t solids = 0;
  std::uint32_t inlets = 0;
  for (const std::uint32_t cell : occupancy) {
    solids += cell == static_cast<std::uint32_t>(WindCell::Solid) ? 1U : 0U;
    inlets += cell == static_cast<std::uint32_t>(WindCell::Inlet) ? 1U : 0U;
  }
  EXPECT_GT(solids, 80U);
  EXPECT_GT(inlets, 20U);
  EXPECT_TRUE(InsideFormulaCar({0.0F, 0.08F, 0.0F}));
  EXPECT_FALSE(InsideFormulaCar({1.4F, 0.4F, 0.0F}));
}

TEST(CarTest, CpMapGoesBlueToRed) {
  const glm::vec3 suction = ColorFromCp(AeroCpMin);
  const glm::vec3 stagnation = ColorFromCp(AeroCpMax);
  EXPECT_GT(suction.z, suction.x);
  EXPECT_GT(stagnation.x, stagnation.z);
  EXPECT_GT(stagnation.x, suction.x);
}

}  // namespace
}  // namespace sph::sim
