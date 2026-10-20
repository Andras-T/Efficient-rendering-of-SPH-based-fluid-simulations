#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Image.hpp"

#include <cstdint>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class Hdri;

class FluidPass {
 public:
  FluidPass(const gpu::Context& context, VkFormat hdrFormat, VkFormat depthFormat,
            const Hdri& hdri);

  void resize(VkExtent2D extent);

  [[nodiscard]] const gpu::Image& rawDepth() const { return rawDepth_; }
  [[nodiscard]] const gpu::Image& thickness() const { return thickness_; }
  [[nodiscard]] const gpu::Image& smoothDepth() const { return smoothDepth_; }
  [[nodiscard]] VkExtent2D thicknessExtent() const { return thickness_.extent(); }

  void prepareTargets(VkCommandBuffer commandBuffer) const;
  void recordDepth(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                   std::uint32_t particleCount, VkImageView hardwareDepth, VkExtent2D extent) const;
  void recordThickness(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                       std::uint32_t particleCount) const;
  void recordSmooth(VkCommandBuffer commandBuffer) const;
  void recordComposite(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms) const;

 private:
  void updateDescriptors();

  const gpu::Context* context_ = nullptr;
  gpu::Unique<VkSampler> sampler_;
  gpu::Unique<VkDescriptorSetLayout> storageLayout_;
  gpu::Unique<VkDescriptorSetLayout> compositeLayout_;
  gpu::Unique<VkDescriptorPool> pool_;
  gpu::Unique<VkPipelineLayout> depthLayout_;
  gpu::Unique<VkPipelineLayout> thicknessLayout_;
  gpu::Unique<VkPipelineLayout> smoothLayout_;
  gpu::Unique<VkPipelineLayout> compositePipelineLayout_;
  gpu::Unique<VkPipeline> depthPipeline_;
  gpu::Unique<VkPipeline> thicknessPipeline_;
  gpu::Unique<VkPipeline> smoothPipeline_;
  gpu::Unique<VkPipeline> compositePipeline_;
  VkDescriptorSet horizontalSet_ = VK_NULL_HANDLE;
  VkDescriptorSet verticalSet_ = VK_NULL_HANDLE;
  VkDescriptorSet refineSet_ = VK_NULL_HANDLE;
  VkDescriptorSet compositeSet_ = VK_NULL_HANDLE;
  gpu::Image rawDepth_;
  gpu::Image thickness_;
  gpu::Image smoothTemp_;
  gpu::Image smoothDepth_;
  VkImageView hdriView_ = VK_NULL_HANDLE;
  VkSampler hdriSampler_ = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace sph
