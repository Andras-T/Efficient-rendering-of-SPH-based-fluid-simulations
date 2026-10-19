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
  params.domainMax = {0.40F, 0.32F, 0.40F};
  params.timeStep = 0.0008F;
  params.substeps = 4;
  params.speedOfSound = 32.0F;
  params.viscosity = 0.40F;

  Scene scene;
  scene.params = params;
  const int nx = 5;
  const int ny = 4;
  const int nz = 5;
  scene.particles.reserve(static_cast<std::size_t>(nx * ny * nz));
  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
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
  const float mean = MeanFluidDensity(scene.particles);
  EXPECT_NEAR(mean, scene.params.restDensity, 0.01F * scene.params.restDensity);
}

}  // namespace
}  // namespace sph::sim
