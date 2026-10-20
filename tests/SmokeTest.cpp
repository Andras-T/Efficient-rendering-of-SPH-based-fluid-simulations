#include "sim/SmokeSolver.hpp"

#include <gtest/gtest.h>

namespace sph::sim {
namespace {

TEST(SmokeTest, DomainIsAClosedBox) {
  const SmokeDomain domain;
  EXPECT_EQ(domain.grid.x * domain.grid.y * domain.grid.z, 32U * 48U * 32U);
  const glm::vec3 max = SmokeDomainMax(domain);
  EXPECT_GT(max.y - domain.origin.y, max.x - domain.origin.x);
  EXPECT_NEAR(max.x - domain.origin.x, domain.cellSize * static_cast<float>(domain.grid.x),
              1.0e-5F);
}

}  // namespace
}  // namespace sph::sim
