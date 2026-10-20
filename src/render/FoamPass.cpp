#include "render/FoamPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {
namespace {

struct FoamPush {
  VkDeviceAddress frame = 0;
  VkDeviceAddress foam = 0;
  std::uint32_t foamCapacity = 0;
  float foamRadius = 0.0F;
};

static_assert(sizeof(FoamPush) == 24);

}  // namespace

FoamPass::FoamPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(FoamPush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);

  const std::array colorFormats{colorFormat};
  pipeline_ =
      gpu::CreateGraphicsPipeline(context.device(), {
                                                        .vertexSpirv = shaders::Find("Foam.vert"),
                                                        .fragmentSpirv = shaders::Find("Foam.frag"),
                                                        .layout = layout_.get(),
                                                        .colorFormats = colorFormats,
                                                        .depthFormat = depthFormat,
                                                        .depthTest = true,
                                                        .depthWrite = false,
                                                        .alphaBlend = true,
                                                    });
}

void FoamPass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkDeviceAddress foam,
                      std::uint32_t foamCapacity, float foamRadius) const {
  if (foamCapacity == 0 || foam == 0) {
    return;
  }
  const FoamPush constants{
      .frame = uniforms,
      .foam = foam,
      .foamCapacity = foamCapacity,
      .foamRadius = foamRadius,
  };
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  vkCmdPushConstants(commandBuffer, layout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, 6, foamCapacity, 0, 0);
}

}  // namespace sph::render
