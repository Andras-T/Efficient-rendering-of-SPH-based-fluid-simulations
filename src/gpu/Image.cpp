#include "gpu/Image.hpp"

#include "gpu/Context.hpp"

#include <utility>

namespace sph::gpu {

Image::Image(const Context& context, const ImageDesc& desc)
    : device_(context.device()),
      allocator_(context.allocator()),
      extent_(desc.extent),
      format_(desc.format) {
  const VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = desc.format,
      .extent = {desc.extent.width, desc.extent.height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = desc.usage,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  const VmaAllocationCreateInfo allocationInfo{.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};
  Check(vmaCreateImage(allocator_, &imageInfo, &allocationInfo, &image_, &allocation_, nullptr),
        "vmaCreateImage");

  const VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = desc.format,
      .subresourceRange = {.aspectMask = desc.aspect, .levelCount = 1, .layerCount = 1},
  };
  const VkResult result = vkCreateImageView(device_, &viewInfo, nullptr, &view_);
  if (result != VK_SUCCESS) {
    reset();
    Check(result, "vkCreateImageView");
  }
}

Image::~Image() { reset(); }

Image::Image(Image&& other) noexcept
    : device_(other.device_),
      allocator_(other.allocator_),
      image_(std::exchange(other.image_, VK_NULL_HANDLE)),
      allocation_(std::exchange(other.allocation_, nullptr)),
      view_(std::exchange(other.view_, VK_NULL_HANDLE)),
      extent_(other.extent_),
      format_(other.format_) {}

Image& Image::operator=(Image&& other) noexcept {
  if (this != &other) {
    reset();
    device_ = other.device_;
    allocator_ = other.allocator_;
    image_ = std::exchange(other.image_, VK_NULL_HANDLE);
    allocation_ = std::exchange(other.allocation_, nullptr);
    view_ = std::exchange(other.view_, VK_NULL_HANDLE);
    extent_ = other.extent_;
    format_ = other.format_;
  }
  return *this;
}

void Image::reset() {
  if (view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(device_, view_, nullptr);
    view_ = VK_NULL_HANDLE;
  }
  if (image_ != VK_NULL_HANDLE) {
    vmaDestroyImage(allocator_, image_, allocation_);
    image_ = VK_NULL_HANDLE;
    allocation_ = nullptr;
  }
}

}  // namespace sph::gpu
