#pragma once

#include "render/BackgroundPass.hpp"

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
  Renderer(const gpu::Context& context, VkFormat colorFormat);

  // Draws the frame and leaves `target` in COLOR_ATTACHMENT_OPTIMAL.
  // `overlay` is recorded last, inside the same rendering scope.
  void record(VkCommandBuffer commandBuffer, const RenderTarget& target,
              const OverlayCallback& overlay = {}) const;

  [[nodiscard]] BackgroundSettings& background() { return background_; }

 private:
  BackgroundPass backgroundPass_;
  BackgroundSettings background_;
};

}  // namespace sph::render
