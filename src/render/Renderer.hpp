#pragma once

#include "gpu/Buffer.hpp"
#include "gpu/Image.hpp"
#include "render/BackgroundPass.hpp"
#include "render/Camera.hpp"
#include "render/ParticlePass.hpp"
#include "sim/SimParams.hpp"

#include <cstdint>
#include <functional>

namespace sph::render {

struct RenderTarget {
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkExtent2D extent{};
};

using OverlayCallback = std::function<void(VkCommandBuffer)>;

class Renderer {
 public:
  static constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

  Renderer(const gpu::Context& context, VkFormat colorFormat);

  void resize(VkExtent2D extent);

  // Draws the frame and leaves `target` in COLOR_ATTACHMENT_OPTIMAL.
  // `overlay` is recorded last, inside the same rendering scope.
  void record(VkCommandBuffer commandBuffer, const RenderTarget& target, const Camera& camera,
              VkDeviceAddress particles, std::uint32_t particleCount, const sim::SimParams& params,
              const OverlayCallback& overlay = {});

  [[nodiscard]] BackgroundSettings& background() { return background_; }

 private:
  const gpu::Context* context_ = nullptr;
  BackgroundPass backgroundPass_;
  ParticlePass particlePass_;
  BackgroundSettings background_;
  gpu::Image depth_;
  gpu::Buffer uniforms_;
};

}  // namespace sph::render
