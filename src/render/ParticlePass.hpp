#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Vulkan.hpp"

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class ParticlePass {
 public:
  ParticlePass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
              std::uint32_t particleCount) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
};

}  // namespace render
}  // namespace sph
