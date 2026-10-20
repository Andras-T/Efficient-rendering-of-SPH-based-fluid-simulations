#include "render/SmokePass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {
namespace {

struct SmokeVolumePush {
  VkDeviceAddress frame = 0;
  VkDeviceAddress scalar = 0;
  VkDeviceAddress velocity = 0;
  glm::uvec3 grid{1, 1, 1};
  float cellSize = 0.05F;
  glm::vec3 origin{0.0F};
  float pad = 0.0F;
};

static_assert(sizeof(SmokeVolumePush) == 56);

}  // namespace

SmokePass::SmokePass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(SmokeVolumePush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const std::array colorFormats{colorFormat};
  pipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("FullscreenTriangle.vert"),
                            .fragmentSpirv = shaders::Find("SmokeVolume.frag"),
                            .layout = layout_.get(),
                            .colorFormats = colorFormats,
                            .depthFormat = depthFormat,
                            .depthTest = false,
                            .depthWrite = false,
                            .alphaBlend = true,
                        });
}

void SmokePass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                       VkDeviceAddress scalar, VkDeviceAddress velocity, glm::uvec3 grid,
                       glm::vec3 origin, float cellSize) const {
  const SmokeVolumePush constants{
      .frame = uniforms,
      .scalar = scalar,
      .velocity = velocity,
      .grid = grid,
      .cellSize = cellSize,
      .origin = origin,
  };
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

}  // namespace sph::render
