#include "CubeMesh.hpp"
#include "app/Headless.hpp"
#include "app/ImageWriter.hpp"
#include "gpu/Context.hpp"
#include "sim/AeroBodies.hpp"
#include "sim/Car.hpp"
#include "sim/Mesh.hpp"
#include "sim/WindProbe.hpp"
#include "sim/WindSolver.hpp"
#include "sim/WindUnits.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <numbers>
#include <vector>

namespace sph {
namespace {

TEST(GpuWindTest, InletStaysFastAndTheCarMakesAWake) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(48);
  const auto cellCount = static_cast<std::size_t>(wind.domain().grid.x) *
                         static_cast<std::size_t>(wind.domain().grid.y) *
                         static_cast<std::size_t>(wind.domain().grid.z);
  std::vector<std::uint32_t> occupancy(cellCount);
  wind.downloadOccupancy(occupancy);
  std::vector<glm::vec4> macro(occupancy.size());
  wind.downloadMacro(macro);
  const glm::uvec3 grid = wind.domain().grid;
  const auto at = [&](std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return x + y * grid.x + z * grid.x * grid.y;
  };
  float inletUx = 0.0F;
  std::uint32_t inletCount = 0;
  for (std::uint32_t y = 2; y + 2 < grid.y; ++y) {
    for (std::uint32_t z = 2; z + 2 < grid.z; ++z) {
      const std::uint32_t index = at(2, y, z);
      if (occupancy[index] != static_cast<std::uint32_t>(sim::WindCell::Inlet) &&
          occupancy[index] != static_cast<std::uint32_t>(sim::WindCell::Fluid)) {
        continue;
      }
      inletUx += macro[index].y;
      ++inletCount;
    }
  }
  ASSERT_GT(inletCount, 0U);
  inletUx /= static_cast<float>(inletCount);
  EXPECT_GT(inletUx, 0.5F * wind.inletSpeed());

  float minUx = inletUx;
  for (std::uint32_t z = 2; z + 2 < grid.z; ++z) {
    for (std::uint32_t y = 2; y + 2 < grid.y; ++y) {
      for (std::uint32_t x = grid.x / 2U; x + 2 < grid.x; ++x) {
        const std::uint32_t index = at(x, y, z);
        if (occupancy[index] != static_cast<std::uint32_t>(sim::WindCell::Fluid)) {
          continue;
        }
        minUx = std::min(minUx, macro[index].y);
      }
    }
  }
  EXPECT_LT(minUx, 0.85F * inletUx);
  EXPECT_TRUE(std::isfinite(minUx));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, TracersStayFinite) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(24);
  std::vector<sim::Particle> tracers(sim::WindSolver::TracerCount);
  wind.downloadTracers(tracers);
  std::uint32_t inside = 0;
  for (const sim::Particle& particle : tracers) {
    EXPECT_TRUE(std::isfinite(particle.position.x));
    EXPECT_TRUE(std::isfinite(particle.velocity.x));
    if (particle.position.x > wind.domainMin().x && particle.position.x < wind.domainMax().x) {
      ++inside;
    }
  }
  EXPECT_GT(inside, tracers.size() / 2);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, TracersSeedTheFullInletFace) {
  const gpu::Context context({.validation = true});
  const sim::WindSolver wind(context);
  std::vector<sim::Particle> tracers(sim::WindSolver::TracerCount);
  wind.downloadTracers(tracers);
  const glm::vec3 origin = wind.domain().origin;
  const float cell = wind.domain().cellSize;
  const glm::vec3 extent = glm::vec3(wind.domain().grid) * cell;
  glm::vec3 mn{1.0e9F};
  glm::vec3 mx{-1.0e9F};
  std::uint32_t onFace = 0;
  for (const sim::Particle& particle : tracers) {
    if (particle.position.x > origin.x + cell * 4.0F) {
      continue;
    }
    ++onFace;
    mn = glm::min(mn, particle.position);
    mx = glm::max(mx, particle.position);
  }
  EXPECT_GT(onFace, tracers.size() / 2);
  EXPECT_GT(mx.y - mn.y, extent.y * 0.72F);
  EXPECT_GT(mx.z - mn.z, extent.z * 0.72F);
  EXPECT_EQ(sim::WindSolver::TracerCount,
            sim::WindSolver::TracerInletY * sim::WindSolver::TracerInletZ * 4U);
  EXPECT_LT(sim::WindSolver::TracerInletY * sim::WindSolver::TracerInletZ, 1200U);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, ParticlesSeedTheFullInletFace) {
  const gpu::Context context({.validation = true});
  const sim::WindSolver wind(context);
  std::vector<sim::Particle> particles(sim::WindSolver::ParticleCount);
  wind.downloadParticles(particles);
  const glm::vec3 origin = wind.domain().origin;
  const float cell = wind.domain().cellSize;
  const glm::vec3 extent = glm::vec3(wind.domain().grid) * cell;
  glm::vec3 mn{1.0e9F};
  glm::vec3 mx{-1.0e9F};
  std::uint32_t onFace = 0;
  for (const sim::Particle& particle : particles) {
    if (particle.position.x > origin.x + cell * 4.0F) {
      continue;
    }
    ++onFace;
    mn = glm::min(mn, particle.position);
    mx = glm::max(mx, particle.position);
  }
  EXPECT_EQ(sim::WindSolver::ParticleCount,
            sim::WindSolver::ParticleInletY * sim::WindSolver::ParticleInletZ);
  EXPECT_GT(sim::WindSolver::ParticleCount, sim::WindSolver::TracerCount);
  EXPECT_GT(onFace, particles.size() / 2);
  EXPECT_GT(mx.y - mn.y, extent.y * 0.72F);
  EXPECT_GT(mx.z - mn.z, extent.z * 0.72F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, ParticlesBounceOnTheSolidMesh) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(64);
  std::vector<sim::Particle> particles(sim::WindSolver::ParticleCount);
  wind.downloadParticles(particles);
  const auto cellCount = static_cast<std::size_t>(wind.domain().grid.x) *
                         static_cast<std::size_t>(wind.domain().grid.y) *
                         static_cast<std::size_t>(wind.domain().grid.z);
  std::vector<std::uint32_t> occupancy(cellCount);
  wind.downloadOccupancy(occupancy);
  const glm::uvec3 grid = wind.domain().grid;
  std::uint32_t inSolid = 0;
  std::uint32_t againstBody = 0;
  const auto occupancyAt = [&](int x, int y, int z) -> std::uint32_t {
    if (x < 0 || y < 0 || z < 0 || x >= static_cast<int>(grid.x) || y >= static_cast<int>(grid.y) ||
        z >= static_cast<int>(grid.z)) {
      return static_cast<std::uint32_t>(sim::WindCell::Wall);
    }
    return occupancy[static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(y) * grid.x +
                     static_cast<std::uint32_t>(z) * grid.x * grid.y];
  };
  for (const sim::Particle& particle : particles) {
    const glm::ivec3 cell =
        glm::clamp(glm::ivec3((particle.position - wind.domain().origin) / wind.domain().cellSize),
                   glm::ivec3(0), glm::ivec3(grid) - 1);
    const std::uint32_t here = occupancyAt(cell.x, cell.y, cell.z);
    if (here == static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        here == static_cast<std::uint32_t>(sim::WindCell::Wall)) {
      ++inSolid;
      continue;
    }
    const bool nextToSolid =
        occupancyAt(cell.x + 1, cell.y, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x - 1, cell.y, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y + 1, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y - 1, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y, cell.z + 1) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y, cell.z - 1) == static_cast<std::uint32_t>(sim::WindCell::Solid);
    if (nextToSolid) {
      ++againstBody;
    }
  }
  EXPECT_LT(inSolid, particles.size() / 200);
  EXPECT_GT(againstBody, 40U);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, TracersLeaveMotionStreaks) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(28);
  std::vector<sim::Particle> tracers(sim::WindSolver::TracerCount);
  wind.downloadTracers(tracers);
  float trail = 0.0F;
  std::uint32_t counted = 0;
  for (const sim::Particle& particle : tracers) {
    const float length = glm::length(particle.position - particle.force);
    if (length < 1.0e-5F) {
      continue;
    }
    trail += length;
    ++counted;
  }
  ASSERT_GT(counted, tracers.size() / 4);
  EXPECT_GT(trail / static_cast<float>(counted), 0.03F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, TracersBounceOnTheSolidMesh) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(64);
  std::vector<sim::Particle> tracers(sim::WindSolver::TracerCount);
  wind.downloadTracers(tracers);
  const auto cellCount = static_cast<std::size_t>(wind.domain().grid.x) *
                         static_cast<std::size_t>(wind.domain().grid.y) *
                         static_cast<std::size_t>(wind.domain().grid.z);
  std::vector<std::uint32_t> occupancy(cellCount);
  wind.downloadOccupancy(occupancy);
  const glm::uvec3 grid = wind.domain().grid;
  std::uint32_t inSolid = 0;
  std::uint32_t againstBody = 0;
  const auto occupancyAt = [&](int x, int y, int z) -> std::uint32_t {
    if (x < 0 || y < 0 || z < 0 || x >= static_cast<int>(grid.x) || y >= static_cast<int>(grid.y) ||
        z >= static_cast<int>(grid.z)) {
      return static_cast<std::uint32_t>(sim::WindCell::Wall);
    }
    return occupancy[static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(y) * grid.x +
                     static_cast<std::uint32_t>(z) * grid.x * grid.y];
  };
  for (const sim::Particle& particle : tracers) {
    const glm::ivec3 cell =
        glm::clamp(glm::ivec3((particle.position - wind.domain().origin) / wind.domain().cellSize),
                   glm::ivec3(0), glm::ivec3(grid) - 1);
    const std::uint32_t here = occupancyAt(cell.x, cell.y, cell.z);
    if (here == static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        here == static_cast<std::uint32_t>(sim::WindCell::Wall)) {
      ++inSolid;
      continue;
    }
    const bool nextToSolid =
        occupancyAt(cell.x + 1, cell.y, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x - 1, cell.y, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y + 1, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y - 1, cell.z) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y, cell.z + 1) ==
            static_cast<std::uint32_t>(sim::WindCell::Solid) ||
        occupancyAt(cell.x, cell.y, cell.z - 1) == static_cast<std::uint32_t>(sim::WindCell::Solid);
    if (nextToSolid) {
      ++againstBody;
    }
  }
  EXPECT_LT(inSolid, tracers.size() / 200);
  EXPECT_GT(againstBody, 40U);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, TracersFollowTheWakeNotStraightLines) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(80);
  std::vector<sim::Particle> tracers(sim::WindSolver::TracerCount);
  wind.downloadTracers(tracers);
  const float midX = 0.5F * (wind.domainMin().x + wind.domainMax().x);
  float lateral = 0.0F;
  std::uint32_t wake = 0;
  for (const sim::Particle& particle : tracers) {
    if (particle.position.x < midX) {
      continue;
    }
    lateral += std::abs(particle.velocity.y) + std::abs(particle.velocity.z);
    ++wake;
  }
  ASSERT_GT(wake, 40U);
  EXPECT_GT(lateral / static_cast<float>(wake), 0.002F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, SurfaceColorsSpanTheCpLegend) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(64);
  wind.refreshAero();
  glm::vec3 mn{1.0F};
  glm::vec3 mx{0.0F};
  bool suction = false;
  bool highPressure = false;
  for (const sim::MeshVertex& vertex : wind.cpuMesh()) {
    mn = glm::min(mn, vertex.color);
    mx = glm::max(mx, vertex.color);
    suction = suction || (vertex.color.z > vertex.color.x + 0.12F);
    highPressure = highPressure || (vertex.color.x > vertex.color.z + 0.12F);
  }
  EXPECT_GT(glm::length(mx - mn), 0.12F);
  EXPECT_TRUE(suction);
  EXPECT_TRUE(highPressure);
  EXPECT_LT(wind.aero().cpMin, 0.0F);
  EXPECT_GT(wind.aero().cpMax, 0.0F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, CarMakesPositiveDragAndFiniteCoefficients) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(64);
  wind.refreshAero();
  const sim::WindAero aero = wind.aero();
  EXPECT_GT(aero.drag, 0.0F);
  EXPECT_GT(aero.cd, 0.0F);
  EXPECT_GT(aero.frontalArea, 0.0F);
  EXPECT_TRUE(std::isfinite(aero.lift));
  EXPECT_TRUE(std::isfinite(aero.cl));
  EXPECT_LT(aero.cd, 20.0F);
  EXPECT_LT(std::abs(aero.cl), 40.0F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, ImportedCubeOccupiesTheTunnel) {
  const gpu::Context context({.validation = true});
  const auto directory = std::filesystem::temp_directory_path() / "sph-gpu-mesh";
  std::filesystem::create_directories(directory);
  const auto cube = test::WriteCubeObj(directory);
  sim::WindSolver wind(context);
  wind.setObstacleMesh(cube);
  wind.step(32);
  auto mesh = sim::LoadObstacleMesh(cube);
  sim::FitMeshToTunnel(mesh, wind.domain());
  const auto occupancy = sim::BuildWindOccupancy(wind.domain(), mesh);
  std::uint32_t solids = 0;
  for (const std::uint32_t cell : occupancy) {
    solids += cell == static_cast<std::uint32_t>(sim::WindCell::Solid) ? 1U : 0U;
  }
  EXPECT_GT(solids, 40U);
  std::vector<glm::vec4> macro(occupancy.size());
  wind.downloadMacro(macro);
  const glm::uvec3 grid = wind.domain().grid;
  float inletUx = 0.0F;
  std::uint32_t inletCount = 0;
  for (std::uint32_t y = 2; y + 2 < grid.y; ++y) {
    for (std::uint32_t z = 2; z + 2 < grid.z; ++z) {
      const std::uint32_t index = 2 + y * grid.x + z * grid.x * grid.y;
      inletUx += macro[index].y;
      ++inletCount;
    }
  }
  ASSERT_GT(inletCount, 0U);
  inletUx /= static_cast<float>(inletCount);
  EXPECT_GT(inletUx, 0.4F * wind.inletSpeed());
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, ProbeReadsInletAndBody) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.setInletSpeed(sim::KmhToLattice(180.0F));
  wind.step(32);
  const auto cellCount = static_cast<std::size_t>(wind.cellCount());
  std::vector<glm::vec4> macros(cellCount);
  std::vector<std::uint32_t> occupancy(cellCount);
  wind.downloadMacro(macros);
  wind.downloadOccupancy(occupancy);
  const glm::vec3 inlet =
      wind.domain().origin + wind.domain().cellSize * glm::vec3(3.5F, 12.5F, 16.5F);
  const sim::WindProbe air =
      sim::SampleWindPoint(wind.domain(), macros, occupancy, wind.inletSpeed(), inlet);
  ASSERT_TRUE(air.hit);
  EXPECT_FALSE(air.onBody);
  EXPECT_GT(air.speedMs, 0.45F * sim::LatticeToMs(wind.inletSpeed()));
  EXPECT_NEAR(air.speedKmh, air.speedMs * sim::MetersPerSecondToKmh, 0.5F);
  EXPECT_TRUE(std::isfinite(air.cp));
  EXPECT_TRUE(std::isfinite(air.pressure));

  glm::vec3 solidCenter = wind.domain().origin;
  bool foundSolid = false;
  const glm::uvec3 grid = wind.domain().grid;
  for (std::uint32_t z = 1; z + 1 < grid.z && !foundSolid; ++z) {
    for (std::uint32_t y = 1; y + 1 < grid.y && !foundSolid; ++y) {
      for (std::uint32_t x = 8; x + 8 < grid.x; ++x) {
        const std::uint32_t index = x + y * grid.x + z * grid.x * grid.y;
        if (occupancy[index] != static_cast<std::uint32_t>(sim::WindCell::Solid)) {
          continue;
        }
        solidCenter =
            wind.domain().origin + wind.domain().cellSize * (glm::vec3(x, y, z) + glm::vec3(0.5F));
        foundSolid = true;
        break;
      }
    }
  }
  ASSERT_TRUE(foundSolid);
  const glm::vec3 origin = solidCenter - glm::vec3(0.55F, 0.0F, 0.0F);
  const sim::WindProbe body = sim::ProbeWindRay(wind.domain(), macros, occupancy, wind.inletSpeed(),
                                                origin, glm::vec3{1.0F, 0.0F, 0.0F});
  ASSERT_TRUE(body.hit);
  EXPECT_TRUE(body.onBody);
  EXPECT_TRUE(std::isfinite(body.cp));
  EXPECT_TRUE(std::isfinite(body.pressure));
  EXPECT_TRUE(std::isfinite(body.speedKmh));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuWindTest, RotatedBodyKeepsFiniteCd) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context, {.loadDefaultCar = false, .advectOverlays = false});
  auto mesh = sim::BuildBenchmarkCube();
  sim::PlaceBluffBody(mesh, wind.domain());
  wind.setObstacle(std::move(mesh));
  wind.setPose({.yaw = 0.5F * std::numbers::pi_v<float>});
  wind.step(48);
  for (int i = 0; i < 16; ++i) {
    wind.refreshAero();
  }
  EXPECT_TRUE(std::isfinite(wind.aero().cd));
  EXPECT_TRUE(std::isfinite(wind.aero().cl));
  EXPECT_GT(wind.aero().cd, 0.0F);
  EXPECT_GT(wind.aero().frontalArea, 0.0F);
  std::vector<std::uint32_t> occupancy(wind.cellCount());
  wind.downloadOccupancy(occupancy);
  EXPECT_GT(sim::MeasureSolidOccupancy(wind.domain(), occupancy).solids, 20U);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

#ifdef SPH_M32_MEDIA
TEST(GpuWindTest, CaptureRotatedBodyStills) {
  const std::filesystem::path media(SPH_M32_MEDIA);
  std::filesystem::create_directories(media);
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{1280, 720};
  const auto yawPixels = RenderOffscreen(context, {.extent = Extent,
                                                   .frames = 400,
                                                   .simulate = true,
                                                   .simulation = SimulationKind::WindTunnel,
                                                   .windVolume = true,
                                                   .windPose = {.yaw = glm::radians(40.0F)}});
  WritePng(media / "m32-yaw.png", Extent, yawPixels);
  const auto pitchPixels = RenderOffscreen(context, {.extent = Extent,
                                                     .frames = 400,
                                                     .simulate = true,
                                                     .simulation = SimulationKind::WindTunnel,
                                                     .windVolume = true,
                                                     .windPose = {.pitch = glm::radians(22.0F)}});
  WritePng(media / "m32-pitch.png", Extent, pitchPixels);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}
#endif

}  // namespace
}  // namespace sph
