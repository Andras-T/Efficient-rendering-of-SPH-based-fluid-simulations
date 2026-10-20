#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Vulkan.hpp"

#include <glm/vec3.hpp>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class CarPass {
 public:
  CarPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkDeviceAddress mesh,
              std::uint32_t vertexCount) const;
  void recordGround(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, glm::vec3 min,
                    glm::vec3 max) const;

 private:
  gpu::Unique<VkPipelineLayout> meshLayout_;
  gpu::Unique<VkPipeline> meshPipeline_;
  gpu::Unique<VkPipelineLayout> groundLayout_;
  gpu::Unique<VkPipeline> groundPipeline_;
};

}  // namespace render
}  // namespace sph
