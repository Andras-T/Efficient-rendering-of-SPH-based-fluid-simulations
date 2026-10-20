#include "sim/Scene.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>

namespace sph::sim {
namespace {

TEST(SceneTest, DefaultViscosityIsPointZeroThreeThree) {
  EXPECT_NEAR(SimParams{}.viscosity, 0.033F, 1.0e-6F);
  EXPECT_NEAR(BuildScene(ScenePreset::Drop).params.viscosity, 0.033F, 1.0e-6F);
}

TEST(SceneTest, MassIsDensityTimesVolume) {
  SimParams params;
  params.particleSpacing = 0.05F;
  params.restDensity = 1000.0F;
  EXPECT_NEAR(params.mass(), 0.125F, 1.0e-6F);
  EXPECT_NEAR(params.supportRadius(), 0.10F, 1.0e-6F);
}

TEST(SceneTest, BoundariesCoverTheBoxShell) {
  const Scene scene = BuildScene(ScenePreset::DamBreak);
  bool floor = false;
  bool wallX = false;
  bool wallZ = false;
  for (const Particle& particle : scene.particles) {
    if (particle.flags != ParticleFlagBoundary) {
      continue;
    }
    EXPECT_GT(particle.mass, 0.0F);
    floor = floor || particle.position.y < scene.params.domainMin.y + scene.params.particleSpacing;
    wallX = wallX ||
            particle.position.x > scene.params.domainMax.x - scene.params.particleSpacing * 1.5F;
    wallZ = wallZ || particle.position.z < scene.params.domainMin.z + scene.params.particleSpacing;
  }
  EXPECT_TRUE(floor);
  EXPECT_TRUE(wallX);
  EXPECT_TRUE(wallZ);
}

TEST(SceneTest, DamBreakFillsLeftSideWithFloor) {
  const Scene scene = BuildScene(ScenePreset::DamBreak);
  ASSERT_FALSE(scene.particles.empty());
  std::uint32_t fluid = 0;
  std::uint32_t boundary = 0;
  float maxFluidX = scene.params.domainMin.x;
  for (const Particle& particle : scene.particles) {
    EXPECT_GE(particle.position.x, scene.params.domainMin.x);
    EXPECT_LE(particle.position.x, scene.params.domainMax.x);
    EXPECT_GE(particle.position.y, scene.params.domainMin.y);
    EXPECT_LE(particle.position.y, scene.params.domainMax.y);
    if (particle.flags == ParticleFlagBoundary) {
      ++boundary;
    } else {
      ++fluid;
      maxFluidX = std::max(maxFluidX, particle.position.x);
    }
  }
  EXPECT_GT(fluid, 0U);
  EXPECT_GT(boundary, 0U);
  EXPECT_LT(maxFluidX, 0.5F * (scene.params.domainMin.x + scene.params.domainMax.x));
}

TEST(SceneTest, DropKeepsARaisedBlobAndAPool) {
  const Scene scene = BuildScene(ScenePreset::Drop);
  float minY = scene.params.domainMax.y;
  float maxY = scene.params.domainMin.y;
  for (const Particle& particle : scene.particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    minY = std::min(minY, particle.position.y);
    maxY = std::max(maxY, particle.position.y);
  }
  EXPECT_GT(maxY - minY, 0.3F);
}

TEST(SceneTest, DoubleDamHasTwoColumns) {
  const Scene scene = BuildScene(ScenePreset::DoubleDam);
  bool left = false;
  bool right = false;
  const float mid = 0.5F * (scene.params.domainMin.x + scene.params.domainMax.x);
  for (const Particle& particle : scene.particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    left = left || particle.position.x < mid - 0.1F;
    right = right || particle.position.x > mid + 0.1F;
  }
  EXPECT_TRUE(left);
  EXPECT_TRUE(right);
}

TEST(SceneTest, EmptyHasNoParticles) {
  EXPECT_TRUE(BuildScene(ScenePreset::Empty).particles.empty());
}

TEST(SceneTest, ParsesPresetNames) {
  EXPECT_EQ(ParseScenePreset("dam"), ScenePreset::DamBreak);
  EXPECT_EQ(ParseScenePreset("drop"), ScenePreset::Drop);
  EXPECT_EQ(ParseScenePreset("double"), ScenePreset::DoubleDam);
  EXPECT_EQ(ParseScenePreset("empty"), ScenePreset::Empty);
  EXPECT_EQ(ParseScenePreset("dropin"), ScenePreset::DropIn);
  EXPECT_EQ(ParseScenePreset("impact"), ScenePreset::DropIn);
  EXPECT_THROW(static_cast<void>(ParseScenePreset("soup")), std::invalid_argument);
}

TEST(SceneTest, DropInPoolSitsBelowTheHangingBlock) {
  const Scene scene = BuildScene(ScenePreset::DropIn);
  const glm::vec3 start = WoodenBlockStart(ScenePreset::DropIn, scene.params);
  std::uint32_t fluid = 0;
  float maxFluidY = scene.params.domainMin.y;
  for (const Particle& particle : scene.particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    ++fluid;
    maxFluidY = std::max(maxFluidY, particle.position.y);
  }
  EXPECT_GT(fluid, 0U);
  EXPECT_GT(start.y, 0.5F * (scene.params.domainMin.y + scene.params.domainMax.y));
  EXPECT_LT(maxFluidY, start.y - WoodenBlockHalf.y);
  EXPECT_STREQ(ScenePresetName(ScenePreset::DropIn), "dropin");
}

TEST(SceneTest, DropInBlockFallsFromTheCeiling) {
  const SimParams params;
  glm::vec3 center = WoodenBlockStart(ScenePreset::DropIn, params);
  glm::vec3 velocity{0.0F};
  const float startY = center.y;
  for (int step = 0; step < 40; ++step) {
    AdvanceWoodenBlock(center, velocity, params);
  }
  EXPECT_LT(center.y, startY);
  EXPECT_LT(velocity.y, 0.0F);
  EXPECT_GE(center.y, params.domainMin.y + WoodenBlockHalf.y);
}

}  // namespace
}  // namespace sph::sim
