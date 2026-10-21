#include "render/WindVolumePass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {
namespace {

struct WindVolumePush {
  VkDeviceAddress frame = 0;
  VkDeviceAddress macro = 0;
  VkDeviceAddress occupancy = 0;
  glm::uvec3 grid{1, 1, 1};
  float cellSize = 0.04F;
  glm::vec3 origin{0.0F};
  float inletUx = 0.08F;
};

static_assert(sizeof(WindVolumePush) == 56);

}  // namespace

WindVolumePass::WindVolumePass(const gpu::Context& context, VkFormat colorFormat,
                               VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(WindVolumePush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const std::array colorFormats{colorFormat};
  pipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("FullscreenTriangle.vert"),
                            .fragmentSpirv = shaders::Find("WindVolume.frag"),
                            .layout = layout_.get(),
                            .colorFormats = colorFormats,
                            .depthFormat = depthFormat,
                            .depthTest = false,
                            .depthWrite = false,
                            .alphaBlend = true,
                        });
}

void WindVolumePass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms,
                            VkDeviceAddress macro, VkDeviceAddress occupancy, glm::uvec3 grid,
                            glm::vec3 origin, float cellSize, float inletUx) const {
  const WindVolumePush constants{
      .frame = uniforms,
      .macro = macro,
      .occupancy = occupancy,
      .grid = grid,
      .cellSize = cellSize,
      .origin = origin,
      .inletUx = inletUx,
  };
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

}  // namespace sph::render
