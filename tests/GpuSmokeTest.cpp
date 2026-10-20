#include "gpu/Context.hpp"
#include "sim/SmokeSolver.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace sph {
namespace {

TEST(GpuSmokeTest, EmitterMakesARisingPlume) {
  const gpu::Context context({.validation = true});
  sim::SmokeSolver smoke(context);
  smoke.step(36);
  std::vector<glm::vec2> scalar(smoke.cellCount());
  std::vector<glm::vec4> velocity(smoke.cellCount());
  smoke.downloadScalar(scalar);
  smoke.downloadVelocity(velocity);
  const glm::uvec3 grid = smoke.domain().grid;
  const auto at = [&](std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return x + y * grid.x + z * grid.x * grid.y;
  };

  float maxDensity = 0.0F;
  float upperDensity = 0.0F;
  float emitterUy = 0.0F;
  std::uint32_t upperCount = 0;
  std::uint32_t emitterCount = 0;
  const std::uint32_t midX = grid.x / 2U;
  const std::uint32_t midZ = grid.z / 2U;
  for (std::uint32_t z = 1; z + 1 < grid.z; ++z) {
    for (std::uint32_t y = 1; y + 1 < grid.y; ++y) {
      for (std::uint32_t x = 1; x + 1 < grid.x; ++x) {
        const std::uint32_t index = at(x, y, z);
        maxDensity = std::max(maxDensity, scalar[index].x);
        if (y > 12U) {
          upperDensity += scalar[index].x;
          ++upperCount;
        }
      }
    }
  }
  for (std::uint32_t y = 2; y < 10; ++y) {
    const std::uint32_t index = at(midX, y, midZ);
    emitterUy += velocity[index].y;
    ++emitterCount;
  }
  ASSERT_GT(upperCount, 0U);
  ASSERT_GT(emitterCount, 0U);
  EXPECT_GT(maxDensity, 0.20F);
  EXPECT_GT(upperDensity / static_cast<float>(upperCount), 0.001F);
  EXPECT_GT(emitterUy / static_cast<float>(emitterCount), 0.02F);
  EXPECT_TRUE(std::isfinite(maxDensity));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
