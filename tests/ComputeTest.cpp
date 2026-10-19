#include "ShaderLibrary.hpp"
#include "gpu/Buffer.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <numeric>

namespace sph {
namespace {

struct SmokeConstants {
  VkDeviceAddress values;
  std::uint32_t count;
};

TEST(ComputeTest, ShaderUpdatesBufferThroughDeviceAddress) {
  const gpu::Context context({.validation = true});

  constexpr std::uint32_t Count = 1000;
  constexpr std::uint32_t GroupSize = 64;
  const gpu::Buffer buffer(
      context,
      {
          .size = Count * sizeof(std::uint32_t),
          .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          .allocationFlags =
              VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      });
  ASSERT_NE(buffer.address(), 0U);
  const auto values = buffer.mapped<std::uint32_t>();
  std::iota(values.begin(), values.end(), 0U);
  buffer.flush();

  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .size = sizeof(SmokeConstants),
  }};
  const auto layout = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const auto pipeline = gpu::CreateComputePipeline(
      context.device(), {.spirv = shaders::Find("ComputeSmoke.comp"), .layout = layout.get()});

  gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
    const SmokeConstants constants{buffer.address(), Count};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.get());
    vkCmdPushConstants(commandBuffer, layout.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, (Count + GroupSize - 1) / GroupSize, 1, 1);
    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_HOST_BIT,
                       VK_ACCESS_2_HOST_READ_BIT);
  });

  buffer.invalidate();
  for (std::uint32_t i = 0; i < Count; ++i) {
    ASSERT_EQ(values[i], (i * 2) + 1) << "index " << i;
  }
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
