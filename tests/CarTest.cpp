#include "sim/Car.hpp"

#include <gtest/gtest.h>

namespace sph::sim {
namespace {

TEST(CarTest, MeshHasABody) {
  const auto mesh = BuildFormulaCarMesh();
  EXPECT_GT(mesh.size(), 200U);
  bool hasRed = false;
  bool hasWheel = false;
  for (const MeshVertex& vertex : mesh) {
    hasRed = hasRed || vertex.color.x > 0.5F;
    hasWheel = hasWheel || vertex.color.x < 0.1F;
  }
  EXPECT_TRUE(hasRed);
  EXPECT_TRUE(hasWheel);
}

TEST(CarTest, OccupancyMarksTheCarAndInlet) {
  const CarDomain domain;
  const auto occupancy = BuildWindOccupancy(domain);
  EXPECT_EQ(occupancy.size(), domain.grid.x * domain.grid.y * domain.grid.z);
  std::uint32_t solids = 0;
  std::uint32_t inlets = 0;
  for (std::uint32_t z = 0; z < domain.grid.z; ++z) {
    for (std::uint32_t y = 0; y < domain.grid.y; ++y) {
      for (std::uint32_t x = 0; x < domain.grid.x; ++x) {
        const std::uint32_t index = x + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
        if (occupancy[index] == static_cast<std::uint32_t>(WindCell::Solid)) {
          ++solids;
        }
        if (occupancy[index] == static_cast<std::uint32_t>(WindCell::Inlet)) {
          ++inlets;
        }
      }
    }
  }
  EXPECT_GT(solids, 80U);
  EXPECT_GT(inlets, 20U);
  EXPECT_TRUE(InsideFormulaCar({0.0F, 0.08F, 0.0F}));
  EXPECT_FALSE(InsideFormulaCar({1.4F, 0.4F, 0.0F}));
}

}  // namespace
}  // namespace sph::sim
