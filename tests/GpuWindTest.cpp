#include "CubeMesh.hpp"
#include "gpu/Context.hpp"
#include "sim/Car.hpp"
#include "sim/Mesh.hpp"
#include "sim/WindSolver.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
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

}  // namespace
}  // namespace sph
