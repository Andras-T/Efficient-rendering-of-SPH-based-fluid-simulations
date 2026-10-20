#include "gpu/Context.hpp"
#include "sim/Car.hpp"
#include "sim/WindSolver.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace sph {
namespace {

TEST(GpuWindTest, InletStaysFastAndTheCarMakesAWake) {
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  wind.step(48);
  const auto occupancy = sim::BuildWindOccupancy(wind.domain());
  std::vector<glm::vec4> macro(occupancy.size());
  wind.downloadMacro(macro);
  const glm::uvec3 grid = wind.domain().grid;
  const auto at = [&](std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return x + y * grid.x + z * grid.x * grid.y;
  };
  const std::uint32_t midY = grid.y / 2U;
  const std::uint32_t midZ = grid.z / 2U;
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

  const std::uint32_t wakeIndex = at(grid.x * 3 / 4, midY, midZ);
  const float wakeUx = macro[wakeIndex].y;
  EXPECT_LT(wakeUx, inletUx);
  EXPECT_TRUE(std::isfinite(wakeUx));
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

}  // namespace
}  // namespace sph
