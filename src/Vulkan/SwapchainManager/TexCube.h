#pragma once

#define GLFW_INCLUDE_VULKAN
#include "Vulkan/BufferManager/BufferManager.h"
#include "Vulkan/CommandPool/CommandPoolManager.h"
#include "GLFW/glfw3.h"
#include "stb_image.h"

struct TexCube {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView imageView;

  const std::string texNames[6] = {
      "\\res\\cubemap\\right.png", "\\res\\cubemap\\left.png",
      "\\res\\cubemap\\front.png", "\\res\\cubemap\\back.png",
      "\\res\\cubemap\\top.png",   "\\res\\cubemap\\bottom.png"};

private:
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height,
                         CommandPoolManager &commandPoolManager,
                         VkQueue &graphicsQueue);

  void createImage(VkImage &image, VkDeviceMemory &memory, uint32_t width,
                   uint32_t height, VkFormat format, VkImageTiling tiling,
                   VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                   VkDevice &device, VkPhysicalDevice &physicalDevice);

  void createImageView(VkImage &image, VkImageView &imageView, VkFormat format,
                       VkImageAspectFlags aspectFlags, VkDevice &device);

public:
  void createTextureImages(VkDevice &device, VkPhysicalDevice &physicalDevice,
                           CommandPoolManager &commandPoolManager,
                           VkQueue &graphicsQueue);

  void cleanUp(VkDevice &device) {
    vkDestroyImage(device, image, nullptr);
    vkDestroyImageView(device, imageView, nullptr);
    vkFreeMemory(device, memory, nullptr);
  }

  VkImage &getImage() { return image; }
  VkDeviceMemory &getImageMemory() { return memory; }
  VkImageView &getImageView() { return imageView; }
};