#include "render/SkyPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"
#include "render/Hdri.hpp"

#include <array>

namespace sph::render {
namespace {

gpu::Unique<VkDescriptorSetLayout> MakeSamplerLayout(VkDevice device) {
  const std::array bindings{VkDescriptorSetLayoutBinding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  }};
  return gpu::CreateDescriptorSetLayout(device, bindings);
}

gpu::Unique<VkDescriptorPool> MakeSamplerPool(VkDevice device) {
  const std::array poolSizes{VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
  }};
  return gpu::CreateDescriptorPool(device, 1, poolSizes);
}

gpu::Unique<VkPipelineLayout> MakeFullscreenLayout(VkDevice device,
                                                   VkDescriptorSetLayout setLayout) {
  const std::array layouts{setLayout};
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(VkDeviceAddress),
  }};
  return gpu::CreatePipelineLayout(device, pushConstants, layouts);
}

}  // namespace

SkyPass::SkyPass(const gpu::Context& context, VkFormat colorFormat, const Hdri& hdri)
    : setLayout_(MakeSamplerLayout(context.device())),
      pool_(MakeSamplerPool(context.device())),
      layout_(MakeFullscreenLayout(context.device(), setLayout_.get())),
      pipeline_(
          gpu::CreateGraphicsPipeline(context.device(),
                                      {
                                          .vertexSpirv = shaders::Find("FullscreenTriangle.vert"),
                                          .fragmentSpirv = shaders::Find("Sky.frag"),
                                          .layout = layout_.get(),
                                          .colorFormats = std::array{colorFormat},
                                      })),
      descriptors_(gpu::AllocateDescriptorSet(context.device(), pool_.get(), setLayout_.get())) {
  gpu::WriteCombinedImage(context.device(), descriptors_, 0, hdri.view(), hdri.sampler());
}

void SkyPass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms) const {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_.get(), 0, 1,
                          &descriptors_, 0, nullptr);
  vkCmdPushConstants(commandBuffer, layout_.get(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(uniforms), &uniforms);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

}  // namespace sph::render
