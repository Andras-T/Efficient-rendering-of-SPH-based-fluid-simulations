#include "BufferManager.h"
#include "../Utils/Structs/Quad.h"
#include "../Utils/Utils.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

void BufferManager::createVertexBuffer(DeviceManager &deviceManager,
                                       VkCommandPool &commandPool,
                                       const void *src, VkBuffer &buffer,
                                       VkDeviceMemory &deviceMemory,
                                       VkDeviceSize bufferSize) {
  VkDevice device = deviceManager.getDevice();
  VkPhysicalDevice physicalDevice = deviceManager.getPhysicalDevice();

  VkBuffer buffer_;
  VkDeviceMemory deviceMemory_;
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer_, deviceMemory_);

  void *bufferData;
  vkMapMemory(device, deviceMemory_, 0, bufferSize, 0, &bufferData);
  memcpy(bufferData, src, (size_t)bufferSize);
  vkUnmapMemory(device, deviceMemory_);

  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);
  copyBuffer(device, commandPool, deviceManager.getGraphicsQueue(), buffer_,
             buffer, bufferSize);

  vkDestroyBuffer(device, buffer_, nullptr);
  vkFreeMemory(device, deviceMemory_, nullptr);
}

void BufferManager::createShaderStorageBuffers(
    DeviceManager &deviceManager, VkCommandPool &commandPool,
    std::vector<VkBuffer> &shaderStorageBuffers,
    std::vector<VkDeviceMemory> &shaderStorageBuffersMemory, glm::vec3 center) {
  VkDevice &device = deviceManager.getDevice();
  VkPhysicalDevice &physicalDevice = deviceManager.getPhysicalDevice();

  if (firstRun) {
    std::vector<glm::vec4> sphereVertices;
    float sphereRadius = Utils::SPHERE_RADIUS;
    int latitudeSegments = 8, longitudeSegments = 8;
    Utils::createSphere(sphereVertices, sphereRadius, latitudeSegments,
                        longitudeSegments);
    VkDeviceSize sphereBufferSize = sizeof(glm::vec4) * sphereVertices.size();
    createVertexBuffer(deviceManager, commandPool, sphereVertices.data(),
                       sphereBuffer, sphereMemory, sphereBufferSize);
    createVertexBuffer(deviceManager, commandPool, quad::quadVertices.data(),
                       quadBuffer, quadMemory,
                       quad::quadVertices.size() * sizeof(float));
    firstRun = false;
  }

  std::vector<Particle> particles(Utils::PARTICLE_COUNT);
  Utils::initializeParticles(particles, center);
  VkDeviceSize bufferSize = sizeof(Particle) * Utils::PARTICLE_COUNT;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, particles.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  shaderStorageBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  shaderStorageBuffersMemory.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(device, physicalDevice, bufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i],
                 shaderStorageBuffersMemory[i]);
    copyBuffer(device, commandPool, deviceManager.getGraphicsQueue(),
               stagingBuffer, (shaderStorageBuffers)[i], bufferSize);
  }

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void BufferManager::createUniformBuffers(
    VkDevice &device, VkPhysicalDevice &physicalDevice,
    std::vector<VkBuffer> &uniformBuffers,
    std::vector<VkDeviceMemory> &uniformBuffersMemory,
    std::vector<void *> &uniformBuffersMapped, VkDeviceSize bufferSize) {
  (uniformBuffers).resize(Utils::MAX_FRAMES_IN_FLIGHT);
  (uniformBuffersMemory).resize(Utils::MAX_FRAMES_IN_FLIGHT);
  (uniformBuffersMapped).resize(Utils::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(device, physicalDevice, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers[i], uniformBuffersMemory[i]);
    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void BufferManager::createBuffer(VkDevice &device,
                                 VkPhysicalDevice &physicalDevice,
                                 VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer &buffer,
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

void BufferManager::copyBuffer(VkDevice &device, VkCommandPool &commandPool,
                               VkQueue &graphicsQueue, VkBuffer srcBuffer,
                               VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}