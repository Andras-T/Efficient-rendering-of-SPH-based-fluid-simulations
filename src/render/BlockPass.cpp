#include "render/BlockPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {
namespace {

struct BlockPush {
  VkDeviceAddress frame = 0;
  glm::vec3 bmin{0.0F};
  glm::vec3 bmax{0.0F};
};

static_assert(sizeof(BlockPush) == 32);

}  // namespace

BlockPass::BlockPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(BlockPush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const std::array colorFormats{colorFormat};
  pipeline_ = gpu::CreateGraphicsPipeline(context.device(),
                                          {
                                              .vertexSpirv = shaders::Find("Block.vert"),
                                              .fragmentSpirv = shaders::Find("Block.frag"),
                                              .layout = layout_.get(),
                                              .colorFormats = colorFormats,
                                              .depthFormat = depthFormat,
                                              .cullMode = VK_CULL_MODE_NONE,
                                              .depthTest = true,
                                              .depthWrite = true,
                                          });
}

void BlockPass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, glm::vec3 min,
                       glm::vec3 max) const {
  if (max.x <= min.x || max.y <= min.y || max.z <= min.z) {
    return;
  }
  const BlockPush constants{.frame = uniforms, .bmin = min, .bmax = max};
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

}  // namespace sph::render
