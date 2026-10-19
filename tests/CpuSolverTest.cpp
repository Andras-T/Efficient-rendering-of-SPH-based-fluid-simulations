#include "sim/CpuSolver.hpp"
#include "sim/Kernels.hpp"
#include "sim/Scene.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace sph::sim {
namespace {

Scene MakeTank() {
  SimParams params;
  params.particleSpacing = 0.08F;
  params.domainMin = {0.0F, 0.0F, 0.0F};
  params.domainMax = {0.64F, 0.64F, 0.64F};
  params.timeStep = 0.0008F;
  params.substeps = 4;
  params.speedOfSound = 32.0F;
  params.viscosity = 0.40F;

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
            (static_cast<float>(x) + 0.5F) * params.particleSpacing,
            (static_cast<float>(y) + 0.5F) * params.particleSpacing,
            (static_cast<float>(z) + 0.5F) * params.particleSpacing,
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

TEST(CpuSolverTest, SettledTankStaysNearRestDensity) {
  Scene scene = MakeTank();
  StepCpu(scene.particles, scene.params, 80);
  ComputeDensityAndPressure(scene.particles, scene.params);
  const float mean = MeanInteriorDensity(scene.particles, scene.params);
  ASSERT_GT(mean, 0.0F);
  EXPECT_NEAR(mean, scene.params.restDensity, 0.01F * scene.params.restDensity);
}

}  // namespace
}  // namespace sph::sim
