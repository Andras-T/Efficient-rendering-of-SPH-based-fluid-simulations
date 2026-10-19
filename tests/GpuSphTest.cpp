#include "gpu/Context.hpp"
#include "sim/CpuSolver.hpp"
#include "sim/Grid.hpp"
#include "sim/Scene.hpp"
#include "sim/SphSolver.hpp"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace sph {
namespace {

sim::Scene MakeCloud() {
  sim::SimParams params;
  params.particleSpacing = 0.08F;
  params.domainMin = {0.0F, 0.0F, 0.0F};
  params.domainMax = {0.40F, 0.40F, 0.40F};
  params.timeStep = 0.0008F;
  params.substeps = 1;
  params.speedOfSound = 32.0F;

  sim::Scene scene;
  scene.params = params;
  constexpr int Extent = 4;
  for (int z = 0; z < Extent; ++z) {
    for (int y = 0; y < Extent; ++y) {
      for (int x = 0; x < Extent; ++x) {
        sim::Particle particle;
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
  sim::CalibrateRestDensity(scene.particles, scene.params);
  return scene;
}

sim::Scene MakeTank() {
  sim::SimParams params;
  params.particleSpacing = 0.08F;
  params.domainMin = {0.0F, 0.0F, 0.0F};
  params.domainMax = {0.40F, 0.32F, 0.40F};
  params.timeStep = 0.0008F;
  params.substeps = 4;
  params.speedOfSound = 32.0F;
  params.viscosity = 0.40F;

  sim::Scene scene;
  scene.params = params;
  const int nx = 5;
  const int ny = 4;
  const int nz = 5;
  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        sim::Particle particle;
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
  sim::CalibrateRestDensity(scene.particles, scene.params);
  return scene;
}

void ExpectMatched(const std::vector<sim::Particle>& gpu, const std::vector<sim::Particle>& cpu,
                   float spacing) {
  ASSERT_EQ(gpu.size(), cpu.size());
  std::vector<char> used(cpu.size(), 0);
  const float maxDistance = 0.35F * spacing;
  for (const sim::Particle& particle : gpu) {
    float best = 1.0e9F;
    int match = -1;
    for (int i = 0; i < static_cast<int>(cpu.size()); ++i) {
      if (used.at(static_cast<std::size_t>(i)) != 0) {
        continue;
      }
      const float distance =
          glm::distance(particle.position, cpu.at(static_cast<std::size_t>(i)).position);
      if (distance < best) {
        best = distance;
        match = i;
      }
    }
    ASSERT_GE(match, 0);
    used.at(static_cast<std::size_t>(match)) = 1;
    const sim::Particle& other = cpu.at(static_cast<std::size_t>(match));
    EXPECT_LT(best, maxDistance);
    EXPECT_NEAR(particle.density, other.density, 0.05F * std::max(other.density, 1.0F));
    EXPECT_NEAR(particle.velocity.x, other.velocity.x, 0.05F);
    EXPECT_NEAR(particle.velocity.y, other.velocity.y, 0.05F);
    EXPECT_NEAR(particle.velocity.z, other.velocity.z, 0.05F);
    EXPECT_TRUE(std::isfinite(particle.position.x));
    EXPECT_TRUE(std::isfinite(particle.density));
  }
}

TEST(GpuSphTest, ExclusiveScanMatchesCpu) {
  const gpu::Context context({.validation = true});
  sim::SphSolver solver(context);

  std::vector<std::uint32_t> counts{3, 1, 0, 4, 8, 2};
  std::vector<std::uint32_t> expected(counts.size());
  sim::ExclusiveScan(counts, expected);
  EXPECT_EQ(solver.exclusiveScan(counts), expected);

  counts.resize(300);
  for (std::uint32_t i = 0; i < counts.size(); ++i) {
    counts[i] = i % 7U;
  }
  expected.resize(counts.size());
  sim::ExclusiveScan(counts, expected);
  EXPECT_EQ(solver.exclusiveScan(counts), expected);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, OneStepMatchesCpuReference) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = MakeCloud();
  std::vector<sim::Particle> cpu = scene.particles;
  sim::StepCpu(cpu, scene.params, 1);

  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  solver.step(1);
  std::vector<sim::Particle> gpu = scene.particles;
  solver.download(gpu);
  ExpectMatched(gpu, cpu, scene.params.particleSpacing);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, SettledTankStaysNearRestDensity) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = MakeTank();
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  solver.step(scene.params.substeps * 20);
  solver.download(scene.particles);
  const float mean = sim::MeanFluidDensity(scene.particles);
  EXPECT_NEAR(mean, scene.params.restDensity, 0.01F * scene.params.restDensity);
  for (const sim::Particle& particle : scene.particles) {
    EXPECT_TRUE(std::isfinite(particle.position.x));
    EXPECT_TRUE(std::isfinite(particle.density));
    EXPECT_GE(particle.position.x, scene.params.domainMin.x);
    EXPECT_LE(particle.position.x, scene.params.domainMax.x);
  }
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
