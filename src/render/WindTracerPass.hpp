#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Vulkan.hpp"

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class WindTracerPass {
 public:
  WindTracerPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, std::uint32_t count,
              bool velocityView) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> smoke_;
  gpu::Unique<VkPipeline> velocity_;
};

}  // namespace render
}  // namespace sph
