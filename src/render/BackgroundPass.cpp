#include "render/BackgroundPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {

BackgroundPass::BackgroundPass(const gpu::Context& context, VkFormat colorFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(BackgroundSettings),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);

  const std::array colorFormats{colorFormat};
  pipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("FullscreenTriangle.vert"),
                            .fragmentSpirv = shaders::Find("Background.frag"),
                            .layout = layout_.get(),
                            .colorFormats = colorFormats,
                        });
}

void BackgroundPass::record(VkCommandBuffer commandBuffer,
                            const BackgroundSettings& settings) const {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(settings), &settings);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

}  // namespace sph::render
