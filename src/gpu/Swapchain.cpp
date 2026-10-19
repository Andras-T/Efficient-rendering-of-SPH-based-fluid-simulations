#include "gpu/Swapchain.hpp"

#include "gpu/Context.hpp"

#include <format>
#include <stdexcept>

namespace sph::gpu {

Swapchain::Swapchain(const Context& context, VkExtent2D extent) : context_(context) {
  build(extent);
}

Swapchain::~Swapchain() {
  destroyViews();
  vkb::destroy_swapchain(swapchain_);
}

void Swapchain::recreate(VkExtent2D extent) { build(extent); }

void Swapchain::build(VkExtent2D extent) {
  vkb::SwapchainBuilder builder(context_.bootstrapDevice(), context_.surface());
  builder.set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
      .add_fallback_format({VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .set_desired_extent(extent.width, extent.height)
      .set_desired_min_image_count(minImageCount_)
      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  if (swapchain_.swapchain != VK_NULL_HANDLE) {
    builder.set_old_swapchain(swapchain_);
  }
  auto result = builder.build();
  if (!result) {
    throw std::runtime_error(std::format("swapchain: {}", result.error().message()));
  }

  destroyViews();
  vkb::destroy_swapchain(swapchain_);
  swapchain_ = result.value();

  auto images = swapchain_.get_images();
  auto views = swapchain_.get_image_views();
  if (!images || !views) {
    throw std::runtime_error("swapchain images unavailable");
  }
  images_ = std::move(images.value());
  views_ = std::move(views.value());
}

void Swapchain::destroyViews() {
  if (!views_.empty()) {
    swapchain_.destroy_image_views(views_);
    views_.clear();
  }
}

}  // namespace sph::gpu
