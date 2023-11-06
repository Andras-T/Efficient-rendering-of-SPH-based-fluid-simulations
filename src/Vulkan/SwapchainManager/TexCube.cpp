#include "TexCube.h"
#include <filesystem>

void TexCube::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                                uint32_t height,
                                CommandPoolManager &commandPoolManager,
                                VkQueue &graphicsQueue) {
  VkCommandBuffer commandBuffer = commandPoolManager.beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 6;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  commandPoolManager.endSingleTimeCommands(commandBuffer, graphicsQueue);
}

void TexCube::createTextureImages(VkDevice &device,
                                  VkPhysicalDevice &physicalDevice,
                                  CommandPoolManager &commandPoolManager,
                                  VkQueue &graphicsQueue) {
  int texWidth, texHeight, texChannels;

  std::string currentPath(std::filesystem::current_path().string());

  stbi_uc *pixels[6];
  int i = 0;
  for (const auto &name : texNames) {
    auto path = currentPath + "\\..\\..\\.." + name;
    pixels[i] = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels,
                          STBI_rgb_alpha);

    if (!pixels[i]) {
      throw std::runtime_error("Failed to load texture image!");
    }
    i++;
  }

  VkDeviceSize vkImageSize = texWidth * texHeight * 4;
  size_t imageSize = static_cast<size_t>(vkImageSize);

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  BufferManager::createBuffer(device, physicalDevice, vkImageSize * 6,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, vkImageSize * 6, 0, &data);
  void *buffer = data;
  for (int i = 0; i < 6; i++) {
    buffer = static_cast<uint8_t *>(data) + imageSize * i;
    memcpy(buffer, pixels[i], imageSize);
    stbi_image_free(pixels[i]);
  }

  vkUnmapMemory(device, stagingBufferMemory);

  createImage(image, memory, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, physicalDevice);

  commandPoolManager.transitionImageLayout(
      image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

  copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight), commandPoolManager,
                    graphicsQueue);

  commandPoolManager.transitionImageLayout(
      image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  createImageView(image, imageView, VK_FORMAT_R8G8B8A8_SRGB,
                  VK_IMAGE_ASPECT_COLOR_BIT, device);
}

void TexCube::createImage(VkImage &image, VkDeviceMemory &memory,
                          uint32_t width, uint32_t height, VkFormat format,
                          VkImageTiling tiling, VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkDevice &device,
                          VkPhysicalDevice &physicalDevice) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 6;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = Utils::findMemoryType(
      memRequirements.memoryTypeBits, properties, physicalDevice);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device, image, memory, 0);
}

void TexCube::createImageView(VkImage &image, VkImageView &imageView,
                              VkFormat format, VkImageAspectFlags aspectFlags,
                              VkDevice &device) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 6;

  if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }
}