#pragma once

#include "gpu/Handles.hpp"

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class TonemapPass {
 public:
  TonemapPass(const gpu::Context& context, VkFormat colorFormat);

  void setSource(VkDevice device, VkImageView hdr, VkSampler sampler);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms) const;

 private:
  gpu::Unique<VkSampler> sampler_;
  gpu::Unique<VkDescriptorSetLayout> setLayout_;
  gpu::Unique<VkDescriptorPool> pool_;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
  VkDescriptorSet descriptors_ = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace sph
