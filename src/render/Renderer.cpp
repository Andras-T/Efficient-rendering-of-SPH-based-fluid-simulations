#include "render/Renderer.hpp"

#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "sim/Particle.hpp"

#include <glm/geometric.hpp>

namespace sph::render {

Renderer::Renderer(const gpu::Context& context, VkFormat colorFormat)
    : context_(&context),
      backgroundPass_(context, colorFormat, DepthFormat),
      particlePass_(context, colorFormat, DepthFormat),
      uniforms_(context,
                {
                    .size = sizeof(sim::FrameUniforms),
                    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT,
                }) {}

void Renderer::resize(VkExtent2D extent) {
  if (extent.width == 0 || extent.height == 0) {
    return;
  }
  if (depth_.handle() != VK_NULL_HANDLE && depth_.extent().width == extent.width &&
      depth_.extent().height == extent.height) {
    return;
  }
  depth_ = gpu::Image(*context_, {
                                     .extent = extent,
                                     .format = DepthFormat,
                                     .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                     .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
                                 });
}

void Renderer::record(VkCommandBuffer commandBuffer, const RenderTarget& target,
                      const Camera& camera, VkDeviceAddress particles, std::uint32_t particleCount,
                      const sim::SimParams& params, const OverlayCallback& overlay) {
  resize(target.extent);

  sim::FrameUniforms uniforms;
  uniforms.view = camera.view();
  uniforms.projection = camera.projection();
  uniforms.lightDirection = glm::normalize(uniforms.lightDirection);
  uniforms.radius = params.renderRadius();
  uniforms.particles = particles;
  uniforms.count = particleCount;
  uniforms_.mapped<sim::FrameUniforms>().front() = uniforms;
  uniforms_.flush();

  gpu::TransitionImage(commandBuffer,
                       {
                           .image = target.image,
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                       });
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = depth_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                           .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                           .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
                       });

  const VkRenderingAttachmentInfo colorAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = target.view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  };
  const VkClearValue depthClear{.depthStencil = {.depth = 0.0F}};
  const VkRenderingAttachmentInfo depthAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth_.view(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = depthClear,
  };
  const VkRect2D area{.extent = target.extent};
  const VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = area,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
  };
  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  const VkViewport viewport{
      .width = static_cast<float>(target.extent.width),
      .height = static_cast<float>(target.extent.height),
      .maxDepth = 1.0F,
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &area);

  backgroundPass_.record(commandBuffer, background_);
  particlePass_.record(commandBuffer, uniforms_.address(), particleCount);
  if (overlay) {
    overlay(commandBuffer);
  }

  vkCmdEndRendering(commandBuffer);
}

}  // namespace sph::render
