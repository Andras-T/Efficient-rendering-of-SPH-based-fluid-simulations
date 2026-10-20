#include "render/FluidPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"
#include "gpu/Vulkan.hpp"
#include "render/Hdri.hpp"

#include <algorithm>
#include <array>

namespace sph::render {
namespace {

constexpr std::int32_t SmoothRadius = 24;
constexpr float SpatialSigma = 16.0F;
constexpr float RangeSigma = 0.28F;
constexpr std::uint32_t SmoothPasses = 2;
constexpr std::uint32_t SmoothTile = 256;

struct SmoothPush {
  std::int32_t horizontal = 1;
  std::int32_t radius = SmoothRadius;
  float spatialSigma = SpatialSigma;
  float rangeSigma = RangeSigma;
};

VkExtent2D HalfExtent(VkExtent2D extent) {
  return {std::max(1U, extent.width / 2), std::max(1U, extent.height / 2)};
}

void SetViewport(VkCommandBuffer commandBuffer, VkExtent2D extent) {
  const VkViewport viewport{
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
      .maxDepth = 1.0F,
  };
  const VkRect2D scissor{.extent = extent};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

gpu::Image MakeColor(const gpu::Context& context, VkExtent2D extent, VkFormat format,
                     VkImageUsageFlags usage) {
  return gpu::Image(context, {
                                 .extent = extent,
                                 .format = format,
                                 .usage = usage,
                             });
}

}  // namespace

FluidPass::FluidPass(const gpu::Context& context, VkFormat hdrFormat, VkFormat depthFormat,
                     const Hdri& hdri)
    : context_(&context), hdriView_(hdri.view()), hdriSampler_(hdri.sampler()) {
  sampler_ = gpu::CreateSampler(context.device(), VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

  const std::array storageBindings{
      VkDescriptorSetLayoutBinding{
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
      VkDescriptorSetLayoutBinding{
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
  };
  storageLayout_ = gpu::CreateDescriptorSetLayout(context.device(), storageBindings);

  const std::array compositeBindings{
      VkDescriptorSetLayoutBinding{
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      VkDescriptorSetLayoutBinding{
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      VkDescriptorSetLayoutBinding{
          .binding = 2,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      VkDescriptorSetLayoutBinding{
          .binding = 3,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
  };
  compositeLayout_ = gpu::CreateDescriptorSetLayout(context.device(), compositeBindings);

  const std::array poolSizes{
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 6},
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 4},
  };
  pool_ = gpu::CreateDescriptorPool(context.device(), 5, poolSizes);

  const std::array particlePush{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(VkDeviceAddress),
  }};
  depthLayout_ = gpu::CreatePipelineLayout(context.device(), particlePush);
  thicknessLayout_ = gpu::CreatePipelineLayout(context.device(), particlePush);

  const std::array storageLayouts{storageLayout_.get()};
  const std::array smoothPush{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .size = sizeof(SmoothPush),
  }};
  smoothLayout_ = gpu::CreatePipelineLayout(context.device(), smoothPush, storageLayouts);

  const std::array compositeSetLayouts{compositeLayout_.get()};
  const std::array compositePush{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(VkDeviceAddress),
  }};
  compositePipelineLayout_ =
      gpu::CreatePipelineLayout(context.device(), compositePush, compositeSetLayouts);

  const std::array depthColor{VK_FORMAT_R32_SFLOAT};
  depthPipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("ParticleBillboard.vert"),
                            .fragmentSpirv = shaders::Find("ParticleDepth.frag"),
                            .layout = depthLayout_.get(),
                            .colorFormats = depthColor,
                            .depthFormat = depthFormat,
                            .depthTest = true,
                            .depthWrite = true,
                        });

  const std::array thicknessColor{VK_FORMAT_R32_SFLOAT};
  thicknessPipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("ParticleBillboard.vert"),
                            .fragmentSpirv = shaders::Find("Thickness.frag"),
                            .layout = thicknessLayout_.get(),
                            .colorFormats = thicknessColor,
                            .additiveBlend = true,
                        });

  smoothPipeline_ =
      gpu::CreateComputePipeline(context.device(), {
                                                       .spirv = shaders::Find("SmoothDepth.comp"),
                                                       .layout = smoothLayout_.get(),
                                                   });

  const std::array hdrColor{hdrFormat};
  compositePipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("FullscreenTriangle.vert"),
                            .fragmentSpirv = shaders::Find("Composite.frag"),
                            .layout = compositePipelineLayout_.get(),
                            .colorFormats = hdrColor,
                        });
}

void FluidPass::resize(VkExtent2D extent) {
  if (extent.width == 0 || extent.height == 0) {
    return;
  }
  if (rawDepth_.handle() != VK_NULL_HANDLE && rawDepth_.extent().width == extent.width &&
      rawDepth_.extent().height == extent.height) {
    return;
  }

  rawDepth_ = MakeColor(*context_, extent, VK_FORMAT_R32_SFLOAT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_STORAGE_BIT);
  thickness_ = MakeColor(*context_, HalfExtent(extent), VK_FORMAT_R32_SFLOAT,
                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  smoothTemp_ = MakeColor(*context_, extent, VK_FORMAT_R32_SFLOAT,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  smoothDepth_ = MakeColor(*context_, extent, VK_FORMAT_R32_SFLOAT,
                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  updateDescriptors();
}

void FluidPass::updateDescriptors() {
  gpu::Check(vkResetDescriptorPool(context_->device(), pool_.get(), 0), "vkResetDescriptorPool");
  horizontalSet_ =
      gpu::AllocateDescriptorSet(context_->device(), pool_.get(), storageLayout_.get());
  verticalSet_ = gpu::AllocateDescriptorSet(context_->device(), pool_.get(), storageLayout_.get());
  refineSet_ = gpu::AllocateDescriptorSet(context_->device(), pool_.get(), storageLayout_.get());
  compositeSet_ =
      gpu::AllocateDescriptorSet(context_->device(), pool_.get(), compositeLayout_.get());

  gpu::WriteStorageImage(context_->device(), horizontalSet_, 0, rawDepth_.view());
  gpu::WriteStorageImage(context_->device(), horizontalSet_, 1, smoothTemp_.view());
  gpu::WriteStorageImage(context_->device(), verticalSet_, 0, smoothTemp_.view());
  gpu::WriteStorageImage(context_->device(), verticalSet_, 1, smoothDepth_.view());
  gpu::WriteStorageImage(context_->device(), refineSet_, 0, smoothDepth_.view());
  gpu::WriteStorageImage(context_->device(), refineSet_, 1, smoothTemp_.view());

  gpu::WriteCombinedImage(context_->device(), compositeSet_, 0, hdriView_, hdriSampler_);
  gpu::WriteCombinedImage(context_->device(), compositeSet_, 1, rawDepth_.view(), sampler_.get());
  gpu::WriteCombinedImage(context_->device(), compositeSet_, 2, smoothDepth_.view(),
                          sampler_.get());
  gpu::WriteCombinedImage(context_->device(), compositeSet_, 3, thickness_.view(), sampler_.get());
}

void FluidPass::prepareTargets(VkCommandBuffer commandBuffer) const {
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = rawDepth_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                       });
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = thickness_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                       });
  gpu::TransitionImage(commandBuffer, {
                                          .image = smoothTemp_.handle(),
                                          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                          .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                                          .dstStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          .dstAccess = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                      });
  gpu::TransitionImage(commandBuffer, {
                                          .image = smoothDepth_.handle(),
                                          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                          .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                                          .dstStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          .dstAccess = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                      });
}

void FluidPass::recordDepth(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                            std::uint32_t particleCount, VkImageView hardwareDepth,
                            VkExtent2D extent) const {
  const VkClearValue colorClear{.color = {.float32 = {0.0F, 0.0F, 0.0F, 0.0F}}};
  const VkClearValue depthClear{.depthStencil = {.depth = 0.0F}};
  const VkRenderingAttachmentInfo colorAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = rawDepth_.view(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = colorClear,
  };
  const VkRenderingAttachmentInfo depthAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = hardwareDepth,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depthClear,
  };
  const VkRect2D area{.extent = extent};
  const VkRenderingInfo rendering{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = area,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
  };
  vkCmdBeginRendering(commandBuffer, &rendering);
  SetViewport(commandBuffer, extent);
  if (particleCount > 0) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPipeline_.get());
    vkCmdPushConstants(commandBuffer, depthLayout_.get(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(uniforms), &uniforms);
    vkCmdDraw(commandBuffer, 6, particleCount, 0, 0);
  }
  vkCmdEndRendering(commandBuffer);
}

void FluidPass::recordThickness(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                                std::uint32_t particleCount) const {
  const VkClearValue colorClear{.color = {.float32 = {0.0F, 0.0F, 0.0F, 0.0F}}};
  const VkRenderingAttachmentInfo colorAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = thickness_.view(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = colorClear,
  };
  const VkExtent2D extent = thickness_.extent();
  const VkRect2D area{.extent = extent};
  const VkRenderingInfo rendering{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = area,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
  };
  vkCmdBeginRendering(commandBuffer, &rendering);
  SetViewport(commandBuffer, extent);
  if (particleCount > 0) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thicknessPipeline_.get());
    vkCmdPushConstants(commandBuffer, thicknessLayout_.get(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(uniforms), &uniforms);
    vkCmdDraw(commandBuffer, 6, particleCount, 0, 0);
  }
  vkCmdEndRendering(commandBuffer);
}

void FluidPass::recordSmooth(VkCommandBuffer commandBuffer) const {
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = rawDepth_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                           .dstAccess = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                       });
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = thickness_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                           .dstAccess = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                       });

  const VkExtent2D extent = rawDepth_.extent();
  const auto groups = [](std::uint32_t value) { return (value + SmoothTile - 1) / SmoothTile; };

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, smoothPipeline_.get());

  const auto blur = [&](VkDescriptorSet horizontalSet) {
    SmoothPush horizontal{.horizontal = 1};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, smoothLayout_.get(), 0,
                            1, &horizontalSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, smoothLayout_.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(horizontal), &horizontal);
    vkCmdDispatch(commandBuffer, groups(extent.width), extent.height, 1);

    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

    SmoothPush vertical{.horizontal = 0};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, smoothLayout_.get(), 0,
                            1, &verticalSet_, 0, nullptr);
    vkCmdPushConstants(commandBuffer, smoothLayout_.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(vertical), &vertical);
    vkCmdDispatch(commandBuffer, groups(extent.height), extent.width, 1);

    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
  };
  blur(horizontalSet_);
  for (std::uint32_t pass = 1; pass < SmoothPasses; ++pass) {
    blur(refineSet_);
  }

  gpu::TransitionImage(commandBuffer, {
                                          .image = rawDepth_.handle(),
                                          .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                                          .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          .srcStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          .srcAccess = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                                          .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                          .dstAccess = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                                      });
  gpu::TransitionImage(commandBuffer, {
                                          .image = smoothDepth_.handle(),
                                          .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                                          .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          .srcStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          .srcAccess = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                          .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                          .dstAccess = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                                      });
}

void FluidPass::recordComposite(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms) const {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline_.get());
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          compositePipelineLayout_.get(), 0, 1, &compositeSet_, 0, nullptr);
  vkCmdPushConstants(commandBuffer, compositePipelineLayout_.get(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(uniforms), &uniforms);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

}  // namespace sph::render
