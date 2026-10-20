#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Vulkan.hpp"

#include <glm/ext/vector_uint3.hpp>
#include <glm/vec3.hpp>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class SmokePass {
 public:
  SmokePass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkDeviceAddress scalar,
              VkDeviceAddress velocity, glm::uvec3 grid, glm::vec3 origin, float cellSize) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
};

}  // namespace render
}  // namespace sph
