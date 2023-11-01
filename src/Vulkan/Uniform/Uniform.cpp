#include "Uniform.h"

Uniform::Uniform(VkDevice &device, VkPhysicalDevice &physicalDevice,
                 VkDeviceSize bufferSize) {
  buffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  buffersMemory.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  buffersMapped.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(device, physicalDevice, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 buffers[i], buffersMemory[i]);
    vkMapMemory(device, buffersMemory[i], 0, bufferSize, 0, &buffersMapped[i]);
  }
}

void Uniform::createBuffer(VkDevice &device, VkPhysicalDevice &physicalDevice,
                           VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = Utils::findMemoryType(
      memRequirements.memoryTypeBits, properties, physicalDevice);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Uniform::cleanUp(VkDevice &device) {
  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, buffers[i], nullptr);
    vkFreeMemory(device, buffersMemory[i], nullptr);
  }
}