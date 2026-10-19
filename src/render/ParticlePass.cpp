#include "render/ParticlePass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {

ParticlePass::ParticlePass(const gpu::Context& context, VkFormat colorFormat,
                           VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(VkDeviceAddress),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);

  const std::array colorFormats{colorFormat};
  pipeline_ = gpu::CreateGraphicsPipeline(context.device(),
                                          {
                                              .vertexSpirv = shaders::Find("Particle.vert"),
                                              .fragmentSpirv = shaders::Find("Particle.frag"),
                                              .layout = layout_.get(),
                                              .colorFormats = colorFormats,
                                              .depthFormat = depthFormat,
                                              .depthTest = true,
                                              .depthWrite = true,
                                          });
}

void ParticlePass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                          std::uint32_t particleCount) const {
  if (particleCount == 0) {
    return;
  }
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uniforms),
                     &uniforms);
  vkCmdDraw(commandBuffer, 6, particleCount, 0, 0);
}

}  // namespace sph::render
