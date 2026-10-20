#pragma once

#include "gpu/Handles.hpp"

#include <glm/vec3.hpp>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class PaddlePass {
 public:
  PaddlePass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, glm::vec3 center,
              float radius) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
};

}  // namespace render
}  // namespace sph
