#pragma once

#define GLFW_INCLUDE_VULKAN
#include "../BufferManager/BufferManager.h"
#include "../CommandPool/CommandPoolManager.h"
#include "GLFW/glfw3.h"

class Image {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView imageView;

public:
  void createImage(uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkDevice &device,
                   VkPhysicalDevice &physicalDevice);

  void createImageView(VkFormat format, VkImageAspectFlags aspectFlags,
                       VkDevice &device);

  VkImage &getImage() { return image; }
  VkDeviceMemory &getImageMemory() { return memory; }
  VkImageView &getImageView() { return imageView; }
};
