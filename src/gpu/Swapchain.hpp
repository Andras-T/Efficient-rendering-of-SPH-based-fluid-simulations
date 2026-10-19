#pragma once

#include "gpu/Vulkan.hpp"

#include <VkBootstrap.h>

#include <cstdint>
#include <vector>

namespace sph::gpu {

class Context;

class Swapchain {
 public:
  Swapchain(const Context& context, VkExtent2D extent);
  ~Swapchain();

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;
  Swapchain(Swapchain&&) = delete;
  Swapchain& operator=(Swapchain&&) = delete;

  // Caller must make sure the old images are no longer in use.
  void recreate(VkExtent2D extent);

  [[nodiscard]] VkSwapchainKHR handle() const { return swapchain_.swapchain; }
  [[nodiscard]] VkFormat format() const { return swapchain_.image_format; }
  [[nodiscard]] VkExtent2D extent() const { return swapchain_.extent; }
  [[nodiscard]] std::uint32_t imageCount() const { return swapchain_.image_count; }
  [[nodiscard]] std::uint32_t minImageCount() const { return minImageCount_; }
  [[nodiscard]] VkImage image(std::uint32_t index) const { return images_.at(index); }
  [[nodiscard]] VkImageView view(std::uint32_t index) const { return views_.at(index); }

 private:
  void build(VkExtent2D extent);
  void destroyViews();

  const Context& context_;
  vkb::Swapchain swapchain_;
  std::vector<VkImage> images_;
  std::vector<VkImageView> views_;
  std::uint32_t minImageCount_ = 2;
};

}  // namespace sph::gpu
