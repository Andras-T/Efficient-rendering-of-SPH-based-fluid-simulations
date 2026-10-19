#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Timestamps.hpp"

#include <gtest/gtest.h>

namespace sph {
namespace {

TEST(TimestampTest, MarksResolveWithoutValidationErrors) {
  const gpu::Context context({.validation = true});
  ASSERT_TRUE(context.timestampsSupported());
  ASSERT_GT(context.timestampPeriod(), 0.0F);

  gpu::GpuProfiler profiler(context, 1);
  ASSERT_TRUE(profiler.supported());

  gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
    profiler.beginFrame(commandBuffer, 0);
    profiler.mark(commandBuffer, "work");
    profiler.endFrame(commandBuffer);
  });
  profiler.resolve(0);

  ASSERT_EQ(profiler.intervals().size(), 1U);
  EXPECT_EQ(profiler.intervals().front().name, "work");
  EXPECT_GE(profiler.intervals().front().milliseconds, 0.0F);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
