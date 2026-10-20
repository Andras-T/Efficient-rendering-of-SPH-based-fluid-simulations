#pragma once

#include "gpu/Handles.hpp"

#include <cstdint>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class Hdri;

class TankPass {
 public:
  TankPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat,
           const Hdri& hdri);

  void recordBack(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                  std::uint32_t vertexCount) const;
  void recordFront(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                   std::uint32_t vertexCount) const;

 private:
  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkPipeline pipeline,
              std::uint32_t vertexCount) const;

  gpu::Unique<VkDescriptorSetLayout> setLayout_;
  gpu::Unique<VkDescriptorPool> pool_;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> backPipeline_;
  gpu::Unique<VkPipeline> frontPipeline_;
  VkDescriptorSet descriptors_ = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace sph
