#include "render/Renderer.hpp"

#include "gpu/Commands.hpp"

namespace sph::render {

Renderer::Renderer(const gpu::Context& context, VkFormat colorFormat)
    : backgroundPass_(context, colorFormat) {}

void Renderer::record(VkCommandBuffer commandBuffer, const RenderTarget& target,
                      const OverlayCallback& overlay) const {
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = target.image,
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                       });

  const VkRenderingAttachmentInfo colorAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = target.view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  };
  const VkRect2D area{.extent = target.extent};
  const VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = area,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
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
  if (overlay) {
    overlay(commandBuffer);
  }

  vkCmdEndRendering(commandBuffer);
}

}  // namespace sph::render
