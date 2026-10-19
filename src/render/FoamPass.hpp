#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Vulkan.hpp"

#include <cstdint>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

class FoamPass {
 public:
  FoamPass(const gpu::Context& context, VkFormat colorFormat, VkFormat depthFormat);

  void record(VkCommandBuffer commandBuffer, VkDeviceAddress uniforms, VkDeviceAddress foam,
              std::uint32_t foamCapacity, float foamRadius) const;

 private:
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> pipeline_;
};

}  // namespace render
}  // namespace sph
