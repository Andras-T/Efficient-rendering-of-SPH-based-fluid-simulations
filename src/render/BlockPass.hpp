#pragma once

#include "gpu/Handles.hpp"

#include <glm/vec3.hpp>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class BlockPass {
 public:
  BlockPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, glm::vec3 min,
              glm::vec3 max) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
};

}  // namespace render
}  // namespace sph
