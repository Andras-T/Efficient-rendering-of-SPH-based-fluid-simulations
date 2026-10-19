#include "sim/Kernels.hpp"
#include "sim/SimParams.hpp"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

namespace sph::sim {
namespace {

TEST(KernelTest, Poly6IntegratesToOne) {
  constexpr float Radius = 0.10F;
  constexpr int Cells = 40;
  const float width = 2.0F * Radius;
  const float step = width / static_cast<float>(Cells);
  double sum = 0.0;
  for (int z = 0; z < Cells; ++z) {
    for (int y = 0; y < Cells; ++y) {
      for (int x = 0; x < Cells; ++x) {
        const glm::vec3 point{
            -Radius + (static_cast<float>(x) + 0.5F) * step,
            -Radius + (static_cast<float>(y) + 0.5F) * step,
            -Radius + (static_cast<float>(z) + 0.5F) * step,
        };
        sum += static_cast<double>(Poly6(glm::dot(point, point), Radius));
      }
    }
  }
  sum *= static_cast<double>(step * step * step);
  EXPECT_NEAR(sum, 1.0, 0.03);
}

TEST(KernelTest, SpikyGradientIsAntisymmetric) {
  const glm::vec3 offset{0.03F, -0.02F, 0.01F};
  const glm::vec3 forward = SpikyGradient(offset, 0.10F);
  const glm::vec3 backward = SpikyGradient(-offset, 0.10F);
  EXPECT_NEAR(forward.x, -backward.x, 1.0e-6F);
  EXPECT_NEAR(forward.y, -backward.y, 1.0e-6F);
  EXPECT_NEAR(forward.z, -backward.z, 1.0e-6F);
}

TEST(KernelTest, TaitPressureVanishesAtRest) {
  EXPECT_EQ(TaitPressure(1000.0F, 1000.0F, 80000.0F, 7.0F), 0.0F);
  EXPECT_EQ(TaitPressure(900.0F, 1000.0F, 80000.0F, 7.0F), 0.0F);
  EXPECT_GT(TaitPressure(1100.0F, 1000.0F, 80000.0F, 7.0F), 0.0F);
}

TEST(KernelTest, StiffnessFromSpeedOfSound) {
  SimParams params;
  params.restDensity = 1000.0F;
  params.speedOfSound = 24.0F;
  params.eosGamma = 7.0F;
  EXPECT_NEAR(params.stiffness(), 1000.0F * 24.0F * 24.0F / 7.0F, 1.0e-3F);
}

}  // namespace
}  // namespace sph::sim
