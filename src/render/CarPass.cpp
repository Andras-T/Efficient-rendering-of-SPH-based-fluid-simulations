#include "render/CarPass.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::render {
namespace {

struct CarPush {
  VkDeviceAddress frame = 0;
  VkDeviceAddress mesh = 0;
};

static_assert(sizeof(CarPush) == 16);

struct GroundPush {
  VkDeviceAddress frame = 0;
  glm::vec3 bmin{0.0F};
  glm::vec3 bmax{0.0F};
};

static_assert(sizeof(GroundPush) == 32);

}  // namespace

CarPass::CarPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat) {
  const std::array meshRange{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(CarPush),
  }};
  meshLayout_ = gpu::CreatePipelineLayout(context.device(), meshRange);
  const std::array groundRange{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .size = sizeof(GroundPush),
  }};
  groundLayout_ = gpu::CreatePipelineLayout(context.device(), groundRange);
  const std::array colorFormats{colorFormat};
  meshPipeline_ =
      gpu::CreateGraphicsPipeline(context.device(), {
                                                        .vertexSpirv = shaders::Find("Car.vert"),
                                                        .fragmentSpirv = shaders::Find("Car.frag"),
                                                        .layout = meshLayout_.get(),
                                                        .colorFormats = colorFormats,
                                                        .depthFormat = depthFormat,
                                                        .cullMode = VK_CULL_MODE_NONE,
                                                        .depthTest = true,
                                                        .depthWrite = true,
                                                    });
  groundPipeline_ = gpu::CreateGraphicsPipeline(
      context.device(), {
                            .vertexSpirv = shaders::Find("WindGround.vert"),
                            .fragmentSpirv = shaders::Find("WindGround.frag"),
                            .layout = groundLayout_.get(),
                            .colorFormats = colorFormats,
                            .depthFormat = depthFormat,
                            .cullMode = VK_CULL_MODE_NONE,
                            .depthTest = true,
                            .depthWrite = true,
                        });
}

void CarPass::record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkDeviceAddress mesh,
                     std::uint32_t vertexCount) const {
  if (vertexCount == 0) {
    return;
  }
  const CarPush constants{.frame = uniforms, .mesh = mesh};
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline_.get());
  vkCmdPushConstants(commandBuffer, meshLayout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void CarPass::recordGround(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, glm::vec3 min,
                           glm::vec3 max) const {
  const GroundPush constants{.frame = uniforms, .bmin = min, .bmax = max};
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, groundPipeline_.get());
  vkCmdPushConstants(commandBuffer, groundLayout_.get(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(constants), &constants);
  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

}  // namespace sph::render
