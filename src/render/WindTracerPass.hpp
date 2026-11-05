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
  void recordParticles(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, std::uint32_t count,
                       bool velocityView) const;

 private:
  void draw(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkDeviceAddress uniforms,
            std::uint32_t count) const;

  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> smoke_;
  gpu::Unique<VkPipeline> velocity_;
  gpu::Unique<VkPipeline> particles_;
  gpu::Unique<VkPipeline> particlesVelocity_;
};

}  // namespace render
}  // namespace sph
