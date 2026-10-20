#include "sim/CpuSolver.hpp"
#include "sim/Kernels.hpp"
#include "sim/Scene.hpp"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace sph::sim {
namespace {

Scene MakeTank() {
  SimParams params;
  params.particleSpacing = 0.08F;
  params.domainMin = {0.0F, 0.0F, 0.0F};
  params.domainMax = {0.72F, 0.72F, 0.72F};
  params.timeStep = 0.0005F;
  params.substeps = 4;
  params.speedOfSound = 32.0F;
  params.viscosity = 0.05F;

  Scene scene;
  scene.params = params;
  constexpr int Extent = 8;
  scene.particles.reserve(static_cast<std::size_t>(Extent) * static_cast<std::size_t>(Extent) *
                          static_cast<std::size_t>(Extent));
  for (int z = 0; z < Extent; ++z) {
    for (int y = 0; y < Extent; ++y) {
      for (int x = 0; x < Extent; ++x) {
        Particle particle;
        particle.position = {
            (static_cast<float>(x) + 1.0F) * params.particleSpacing,
            (static_cast<float>(y) + 1.0F) * params.particleSpacing,
            (static_cast<float>(z) + 1.0F) * params.particleSpacing,
        };
        particle.mass = params.mass();
        particle.density = params.restDensity;
        scene.particles.push_back(particle);
      }
    }
  }
  CalibrateRestDensity(scene.particles, scene.params);
  return scene;
}

TEST(CpuSolverTest, ParticlesStayInTheBox) {
  Scene scene = MakeTank();
  StepCpu(scene.particles, scene.params, 16);
  const glm::vec3 min = scene.params.domainMin;
  const glm::vec3 max = scene.params.domainMax;
  for (const Particle& particle : scene.particles) {
    EXPECT_TRUE(std::isfinite(particle.position.x));
    EXPECT_TRUE(std::isfinite(particle.density));
    EXPECT_GE(particle.position.x, min.x);
    EXPECT_LE(particle.position.x, max.x);
    EXPECT_GE(particle.position.y, min.y);
    EXPECT_LE(particle.position.y, max.y);
    EXPECT_GE(particle.position.z, min.z);
    EXPECT_LE(particle.position.z, max.z);
  }
}

TEST(CpuSolverTest, ClosedBoxStopsUpwardSplash) {
  SimParams params;
  params.particleSpacing = 0.08F;
  params.domainMin = {0.0F, 0.0F, 0.0F};
  params.domainMax = {0.5F, 0.5F, 0.5F};
  params.timeStep = 0.0005F;
  Particle particle;
  particle.position = {0.25F, 0.35F, 0.25F};
  particle.velocity = {0.0F, 40.0F, 0.0F};
  particle.mass = params.mass();
  particle.density = params.restDensity;
  std::vector<Particle> particles{particle};
  StepCpu(particles, params, 80);
  EXPECT_LE(particles.front().position.y, params.domainMax.y);
  EXPECT_GE(particles.front().position.y, params.domainMin.y);
  EXPECT_LT(particles.front().position.y, params.domainMax.y - params.collisionRadius() - 0.01F);
}

TEST(CpuSolverTest, DamBreakStaysInsideClosedTank) {
  Scene scene = BuildScene(ScenePreset::DamBreak);
  StepCpu(scene.particles, scene.params, 12);
  std::uint32_t lidStuck = 0;
  const float lid = scene.params.domainMax.y - scene.params.collisionRadius() * 0.25F;
  for (const Particle& particle : scene.particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    EXPECT_LE(particle.position.x, scene.params.domainMax.x);
    EXPECT_GE(particle.position.x, scene.params.domainMin.x);
    EXPECT_LE(particle.position.y, scene.params.domainMax.y);
    EXPECT_GE(particle.position.y, scene.params.domainMin.y);
    EXPECT_LE(particle.position.z, scene.params.domainMax.z);
    EXPECT_GE(particle.position.z, scene.params.domainMin.z);
    if (particle.position.y > lid && std::abs(particle.velocity.y) < 0.02F) {
      ++lidStuck;
    }
  }
  EXPECT_EQ(lidStuck, 0U);
}

TEST(CpuSolverTest, SettledTankStaysNearRestDensity) {
  Scene scene = MakeTank();
  StepCpu(scene.particles, scene.params, 80);
  ComputeDensityAndPressure(scene.particles, scene.params);
  const float mean = MeanInteriorDensity(scene.particles, scene.params);
  ASSERT_GT(mean, 0.0F);
  EXPECT_NEAR(mean, scene.params.restDensity, 0.01F * scene.params.restDensity);
}

TEST(CpuSolverTest, DfsphSettledTankStaysNearRestDensity) {
  Scene scene = MakeTank();
  scene.params.solver = SolverMethod::Dfsph;
  StepCpu(scene.particles, scene.params, 80);
  ComputeDensityAndPressure(scene.particles, scene.params);
  const float mean = MeanInteriorDensity(scene.particles, scene.params);
  ASSERT_GT(mean, 0.0F);
  EXPECT_NEAR(mean, scene.params.restDensity, 0.04F * scene.params.restDensity);
  for (const Particle& particle : scene.particles) {
    EXPECT_TRUE(std::isfinite(particle.position.x));
    EXPECT_TRUE(std::isfinite(particle.velocity.x));
    EXPECT_LT(glm::length(particle.velocity), 4.0F);
  }
}

TEST(CpuSolverTest, CollisionKeepsFluidOffBoundaryParticles) {
  Scene scene = BuildScene(ScenePreset::DamBreak);
  const float minGap = 0.25F * scene.params.particleSpacing;
  ComputeDensityAndPressure(scene.particles, scene.params);
  StepCpu(scene.particles, scene.params, 4);
  float minBoundDist = 1.0e9F;
  for (const Particle& fluid : scene.particles) {
    if (fluid.flags != ParticleFlagFluid) {
      continue;
    }
    EXPECT_TRUE(std::isfinite(fluid.position.x));
    EXPECT_TRUE(std::isfinite(fluid.velocity.x));
    for (const Particle& wall : scene.particles) {
      if (wall.flags != ParticleFlagBoundary) {
        continue;
      }
      minBoundDist = std::min(minBoundDist, glm::distance(fluid.position, wall.position));
    }
  }
  EXPECT_GT(minBoundDist, minGap);
}

}  // namespace
}  // namespace sph::sim
