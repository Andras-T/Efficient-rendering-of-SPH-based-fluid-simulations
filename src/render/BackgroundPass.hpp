#pragma once

#include "gpu/Handles.hpp"

#include <glm/vec4.hpp>

namespace sph::gpu {
class Context;
}

namespace sph::render {

struct BackgroundSettings {
  glm::vec4 topColor{0.10F, 0.18F, 0.32F, 1.0F};
  glm::vec4 bottomColor{0.55F, 0.70F, 0.85F, 1.0F};
};

// Vertical gradient drawn with a single fullscreen triangle.
class BackgroundPass {
 public:
  BackgroundPass(const gpu::Context& context, VkFormat colorFormat);

  // Must be recorded inside a rendering scope with viewport and scissor set.
  void record(VkCommandBuffer commandBuffer, const BackgroundSettings& settings) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
};

}  // namespace sph::render
