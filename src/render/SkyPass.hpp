#pragma once

#include "gpu/Handles.hpp"

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class Hdri;

class SkyPass {
 public:
  SkyPass(const gpu::Context& context, VkFormat colorFormat, const Hdri& hdri);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms) const;

 private:
  gpu::Unique<VkDescriptorSetLayout> setLayout_;
  gpu::Unique<VkDescriptorPool> pool_;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
  VkDescriptorSet descriptors_ = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace sph
