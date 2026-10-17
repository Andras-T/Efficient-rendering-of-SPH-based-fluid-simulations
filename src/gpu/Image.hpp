#pragma once

#include "gpu/Vulkan.hpp"

namespace sph::gpu {

class Context;

struct ImageDesc {
  VkExtent2D extent{};
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkImageUsageFlags usage = 0;
  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
};

// A device-local 2D image with a matching view.
class Image {
 public:
  Image() = default;
  Image(const Context& context, const ImageDesc& desc);
  ~Image();

  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
  Image(Image&& other) noexcept;
  Image& operator=(Image&& other) noexcept;

  [[nodiscard]] VkImage handle() const { return image_; }
  [[nodiscard]] VkImageView view() const { return view_; }
  [[nodiscard]] VkExtent2D extent() const { return extent_; }
  [[nodiscard]] VkFormat format() const { return format_; }

 private:
  void reset();

  VkDevice device_ = VK_NULL_HANDLE;
  VmaAllocator allocator_ = nullptr;
  VkImage image_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = nullptr;
  VkImageView view_ = VK_NULL_HANDLE;
  VkExtent2D extent_{};
  VkFormat format_ = VK_FORMAT_UNDEFINED;
};

}  // namespace sph::gpu
