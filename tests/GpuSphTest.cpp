#include "gpu/Context.hpp"
#include "sim/CpuSolver.hpp"
#include "sim/Grid.hpp"
#include "sim/Scene.hpp"
#include "sim/SphSolver.hpp"
#include "sim/Tank.hpp"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace sph {
namespace {

sim::Scene MakeCloud() {
  sim::SimParams params;
  params.particleSpacing = 0.08F;
  params.domainMin = {-0.12F, -0.12F, -0.12F};
  params.domainMax = {0.52F, 0.52F, 0.52F};
  params.timeStep = 0.0005F;
  params.substeps = 1;
  params.speedOfSound = 32.0F;
  params.viscosity = 0.05F;

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
  params.domainMax = {0.72F, 0.72F, 0.72F};
  params.timeStep = 0.0005F;
  params.substeps = 4;
  params.speedOfSound = 32.0F;
  params.viscosity = 0.05F;

  sim::Scene scene;
  scene.params = params;
  constexpr int Extent = 8;
  for (int z = 0; z < Extent; ++z) {
    for (int y = 0; y < Extent; ++y) {
      for (int x = 0; x < Extent; ++x) {
        sim::Particle particle;
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

float MeanVelocityX(const std::vector<sim::Particle>& particles) {
  double sum = 0.0;
  std::uint32_t count = 0;
  for (const sim::Particle& particle : particles) {
    if (particle.flags != sim::ParticleFlagFluid) {
      continue;
    }
    sum += static_cast<double>(particle.velocity.x);
    ++count;
  }
  return count == 0 ? 0.0F : static_cast<float>(sum / static_cast<double>(count));
}

TEST(GpuSphTest, CursorForcePushesNearbyFluid) {
  const gpu::Context context({.validation = true});
  const sim::Scene scene = MakeCloud();

  sim::SphSolver baseline(context);
  baseline.upload(scene.particles, scene.params);
  baseline.step(1);
  std::vector<sim::Particle> without = scene.particles;
  baseline.download(without);

  sim::SphSolver stirred(context);
  stirred.upload(scene.particles, scene.params);
  stirred.setCursor(scene.particles.front().position, {80.0F, 0.0F, 0.0F}, 0.5F);
  stirred.step(1);
  std::vector<sim::Particle> withForce = scene.particles;
  stirred.download(withForce);

  EXPECT_GT(MeanVelocityX(withForce), MeanVelocityX(without) + 0.01F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, CursorSwirlSpinsNearbyFluid) {
  const gpu::Context context({.validation = true});
  const sim::Scene scene = MakeCloud();
  const glm::vec3 center = scene.particles.front().position;

  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  solver.setCursor(center, {0.0F, 0.0F, 0.0F}, 0.5F, 40.0F);
  solver.step(1);
  std::vector<sim::Particle> spun = scene.particles;
  solver.download(spun);

  float maxTangent = 0.0F;
  for (const sim::Particle& particle : spun) {
    const glm::vec3 radial = particle.position - center;
    const glm::vec3 tangent = glm::cross(glm::vec3(0.0F, 1.0F, 0.0F), radial);
    if (glm::length(tangent) < 1.0e-6F) {
      continue;
    }
    maxTangent = std::max(maxTangent, glm::dot(particle.velocity, glm::normalize(tangent)));
  }
  EXPECT_GT(maxTangent, 0.01F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, PaddleSphereDisplacesFluid) {
  const gpu::Context context({.validation = true});
  const sim::Scene scene = MakeCloud();
  const glm::vec3 center = scene.particles.front().position;

  sim::SphSolver baseline(context);
  baseline.upload(scene.particles, scene.params);
  baseline.step(1);
  std::vector<sim::Particle> without = scene.particles;
  baseline.download(without);

  sim::SphSolver shoved(context);
  shoved.upload(scene.particles, scene.params);
  shoved.setCursor(center, {0.0F, 0.0F, 0.0F}, 0.18F, 0.0F, 300.0F);
  shoved.step(1);
  std::vector<sim::Particle> withPaddle = scene.particles;
  shoved.download(withPaddle);

  float maxSep = 0.0F;
  for (std::size_t i = 0; i < withPaddle.size(); ++i) {
    maxSep = std::max(maxSep, glm::length(withPaddle[i].position - without[i].position));
  }
  EXPECT_GT(maxSep, 0.005F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, DfsphOneStepMatchesCpuReference) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = MakeCloud();
  scene.params.solver = sim::SolverMethod::Dfsph;
  scene.params.densityIterations = 2;
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

TEST(GpuSphTest, DamBreakStaysFinite) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = sim::BuildScene(sim::ScenePreset::DamBreak);
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);

  constexpr int Frames = 180;
  float maxSpeed = 0.0F;
  float maxDensity = 0.0F;
  for (int frame = 1; frame <= Frames; ++frame) {
    solver.step(scene.params.substeps);
    solver.download(scene.particles);
    for (const sim::Particle& particle : scene.particles) {
      if (particle.flags != sim::ParticleFlagFluid) {
        continue;
      }
      ASSERT_TRUE(std::isfinite(particle.position.x)) << "NaN at frame " << frame;
      ASSERT_TRUE(std::isfinite(particle.velocity.x)) << "NaN at frame " << frame;
      ASSERT_TRUE(std::isfinite(particle.density)) << "NaN at frame " << frame;
      maxSpeed = std::max(maxSpeed, glm::length(particle.velocity));
      maxDensity = std::max(maxDensity, particle.density);
      EXPECT_GE(particle.position.x, scene.params.domainMin.x - 1.0e-3F);
      EXPECT_LE(particle.position.x, scene.params.domainMax.x + 1.0e-3F);
      EXPECT_GE(particle.position.y, scene.params.domainMin.y - 1.0e-3F);
      EXPECT_LE(particle.position.y, scene.params.domainMax.y + 1.0e-3F);
      EXPECT_GE(particle.position.z, scene.params.domainMin.z - 1.0e-3F);
      EXPECT_LE(particle.position.z, scene.params.domainMax.z + 1.0e-3F);
    }
  }

  float minBoundDist = 1.0e9F;
  for (const sim::Particle& fluid : scene.particles) {
    if (fluid.flags != sim::ParticleFlagFluid) {
      continue;
    }
    for (const sim::Particle& wall : scene.particles) {
      if (wall.flags != sim::ParticleFlagBoundary) {
        continue;
      }
      minBoundDist = std::min(minBoundDist, glm::distance(fluid.position, wall.position));
    }
  }
  EXPECT_GT(minBoundDist, 0.25F * scene.params.particleSpacing);
  EXPECT_LT(maxSpeed, 20.0F);
  EXPECT_LT(maxDensity, 1.45F * scene.params.restDensity);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, DfsphDamBreakStaysFinite) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = sim::BuildScene(sim::ScenePreset::DamBreak);
  scene.params.solver = sim::SolverMethod::Dfsph;
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);

  constexpr int Frames = 60;
  float maxSpeed = 0.0F;
  for (int frame = 1; frame <= Frames; ++frame) {
    solver.step(scene.params.substeps);
    solver.download(scene.particles);
    for (const sim::Particle& particle : scene.particles) {
      if (particle.flags != sim::ParticleFlagFluid) {
        continue;
      }
      ASSERT_TRUE(std::isfinite(particle.position.x)) << "NaN at frame " << frame;
      ASSERT_TRUE(std::isfinite(particle.velocity.x)) << "NaN at frame " << frame;
      ASSERT_TRUE(std::isfinite(particle.density)) << "NaN at frame " << frame;
      maxSpeed = std::max(maxSpeed, glm::length(particle.velocity));
      EXPECT_GE(particle.position.y, scene.params.domainMin.y - 1.0e-3F);
      EXPECT_LE(particle.position.y, scene.params.domainMax.y + 1.0e-3F);
    }
  }
  EXPECT_LT(maxSpeed, 20.0F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, CylinderDamBreakStaysInside) {
  const gpu::Context context({.validation = true});
  sim::SimParams params;
  params.tankShape = sim::TankShape::Cylinder;
  sim::Scene scene = sim::BuildScene(sim::ScenePreset::DamBreak, params);
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);

  constexpr int Frames = 60;
  const glm::vec2 center = sim::TankCenterXz(scene.params);
  const float limit = sim::TankRadius(scene.params) + 1.0e-3F;
  for (int frame = 1; frame <= Frames; ++frame) {
    solver.step(scene.params.substeps);
    solver.download(scene.particles);
    for (const sim::Particle& particle : scene.particles) {
      if (particle.flags != sim::ParticleFlagFluid) {
        continue;
      }
      ASSERT_TRUE(std::isfinite(particle.position.x)) << "NaN at frame " << frame;
      const float r = glm::length(glm::vec2(particle.position.x, particle.position.z) - center);
      EXPECT_LE(r, limit) << "frame " << frame;
      EXPECT_GE(particle.position.y, scene.params.domainMin.y - 1.0e-3F);
      EXPECT_LE(particle.position.y, scene.params.domainMax.y + 1.0e-3F);
    }
  }
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, BlockSampleSeesThePool) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = sim::BuildScene(sim::ScenePreset::DropIn);
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  const glm::vec3 half = sim::WoodenBlockHalf;
  glm::vec3 center{0.0F, 0.30F, 0.0F};
  solver.setBlock(center - half, center + half, true);
  solver.step(1);
  EXPECT_GT(solver.blockWaterSurface(), 0.18F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, DropInBlockFloatsInsteadOfSinking) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = sim::BuildScene(sim::ScenePreset::DropIn);
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  const glm::vec3 half = sim::WoodenBlockHalf;
  glm::vec3 center = sim::WoodenBlockStart(sim::ScenePreset::DropIn, scene.params);
  glm::vec3 velocity{0.0F};
  solver.setBlock(center - half, center + half, true);

  constexpr int Frames = 180;
  for (int frame = 0; frame < Frames; ++frame) {
    if (frame > 0) {
      sim::AdvanceWoodenBlock(center, velocity, solver.params(), solver.blockWaterSurface(), true);
      solver.setBlock(center - half, center + half, true);
    }
    solver.step(scene.params.substeps);
  }
  EXPECT_TRUE(std::isfinite(center.y));
  EXPECT_GT(center.y, scene.params.domainMin.y + half.y + 0.06F)
      << "surface " << solver.blockWaterSurface();
  EXPECT_LT(center.y, scene.params.domainMax.y - half.y);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuSphTest, SettledTankStaysNearRestDensity) {
  const gpu::Context context({.validation = true});
  sim::Scene scene = MakeTank();
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  solver.step(scene.params.substeps * 20);
  solver.download(scene.particles);
  sim::ComputeDensityAndPressure(scene.particles, scene.params);
  const float mean = sim::MeanInteriorDensity(scene.particles, scene.params);
  ASSERT_GT(mean, 0.0F);
  EXPECT_NEAR(mean, scene.params.restDensity, 0.01F * scene.params.restDensity);
  for (const sim::Particle& particle : scene.particles) {
    EXPECT_TRUE(std::isfinite(particle.position.x));
    EXPECT_TRUE(std::isfinite(particle.density));
    EXPECT_GE(particle.position.x, scene.params.domainMin.x);
    EXPECT_LE(particle.position.x, scene.params.domainMax.x);
    EXPECT_GE(particle.position.y, scene.params.domainMin.y);
    EXPECT_LE(particle.position.y, scene.params.domainMax.y);
    EXPECT_GE(particle.position.z, scene.params.domainMin.z);
    EXPECT_LE(particle.position.z, scene.params.domainMax.z);
  }
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
