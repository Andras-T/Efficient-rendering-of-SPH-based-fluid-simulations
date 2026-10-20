#include "render/WindTracerPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {

WindTracerPass::WindTracerPass(const gpu::Context& context, VkFormat colorFormat,
                               VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(VkDeviceAddress),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const std::array colorFormats{colorFormat};
  smoke_ = gpu::CreateGraphicsPipeline(context.device(),
                                       {
                                           .vertexSpirv = shaders::Find("WindTracer.vert"),
                                           .fragmentSpirv = shaders::Find("WindTracer.frag"),
                                           .layout = layout_.get(),
                                           .colorFormats = colorFormats,
                                           .depthFormat = depthFormat,
                                           .depthTest = true,
                                           .depthWrite = false,
                                           .alphaBlend = true,
                                       });
  velocity_ = gpu::CreateGraphicsPipeline(context.device(),
                                          {
                                              .vertexSpirv = shaders::Find("WindTracer.vert"),
                                              .fragmentSpirv = shaders::Find("WindTracer.frag"),
                                              .layout = layout_.get(),
                                              .colorFormats = colorFormats,
                                              .depthFormat = depthFormat,
                                              .depthTest = true,
                                              .depthWrite = true,
                                          });
}

void WindTracerPass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                            std::uint32_t count, bool velocityView) const {
  if (count == 0) {
    return;
  }
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    velocityView ? velocity_.get() : smoke_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uniforms),
                     &uniforms);
  vkCmdDraw(commandBuffer, 6, count, 0, 0);
}

}  // namespace sph::render
