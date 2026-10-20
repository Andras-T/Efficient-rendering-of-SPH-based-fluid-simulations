#include "render/TankPass.hpp"

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

}  // namespace

TankPass::TankPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat,
                   const Hdri& hdri)
    : setLayout_(MakeSamplerLayout(context.device())),
      pool_(MakeSamplerPool(context.device())),
      descriptors_(gpu::AllocateDescriptorSet(context.device(), pool_.get(), setLayout_.get())) {
  const std::array layouts{setLayout_.get()};
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(VkDeviceAddress),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants, layouts);
  gpu::WriteCombinedImage(context.device(), descriptors_, 0, hdri.view(), hdri.sampler());

  const std::array colorFormats{colorFormat};
  gpu::GraphicsPipelineDesc desc{
      .vertexSpirv = shaders::Find("Tank.vert"),
      .fragmentSpirv = shaders::Find("Tank.frag"),
      .layout = layout_.get(),
      .colorFormats = colorFormats,
      .depthFormat = depthFormat,
      .cullMode = VK_CULL_MODE_FRONT_BIT,
      .depthTest = true,
      .depthWrite = false,
      .alphaBlend = true,
  };
  backPipeline_ = gpu::CreateGraphicsPipeline(context.device(), desc);
  desc.cullMode = VK_CULL_MODE_BACK_BIT;
  frontPipeline_ = gpu::CreateGraphicsPipeline(context.device(), desc);
}

void TankPass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkPipeline pipeline,
                      std::uint32_t vertexCount) const {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_.get(), 0, 1,
                          &descriptors_, 0, nullptr);
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uniforms),
                     &uniforms);
  vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void TankPass::recordBack(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                          std::uint32_t vertexCount) const {
  record(commandBuffer, uniforms, backPipeline_.get(), vertexCount);
}

void TankPass::recordFront(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                           std::uint32_t vertexCount) const {
  record(commandBuffer, uniforms, frontPipeline_.get(), vertexCount);
}

}  // namespace sph::render
