#include "render/PaddlePass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {
namespace {

struct PaddlePush {
  VkDeviceAddress frame = 0;
  glm::vec3 center{0.0F};
  float radius = 0.0F;
};

static_assert(sizeof(PaddlePush) == 24);

}  // namespace

PaddlePass::PaddlePass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(PaddlePush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const std::array colorFormats{colorFormat};
  pipeline_ = gpu::CreateGraphicsPipeline(context.device(),
                                          {
                                              .vertexSpirv = shaders::Find("Paddle.vert"),
                                              .fragmentSpirv = shaders::Find("Paddle.frag"),
                                              .layout = layout_.get(),
                                              .colorFormats = colorFormats,
                                              .depthFormat = depthFormat,
                                              .depthTest = true,
                                              .depthWrite = false,
                                              .alphaBlend = true,
                                          });
}

void PaddlePass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, glm::vec3 center,
                        float radius) const {
  if (radius <= 0.0F) {
    return;
  }
  const PaddlePush constants{.frame = uniforms, .center = center, .radius = radius};
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

}  // namespace sph::render
