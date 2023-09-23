#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm/vec3.hpp"
#include <vector>

#include "../DeviceManager/DeviceManager.h"

class BufferManager {

  bool singletonCreated = false;

  VkBuffer sphereBuffer;
  VkDeviceMemory sphereMemory;

  VkBuffer quadBuffer;
  VkDeviceMemory quadMemory;

public:
  void createVertexBuffer(DeviceManager &deviceManager,
                          VkCommandPool &commandPool, const void *src,
                          VkBuffer &buffer, VkDeviceMemory &deviceMemory,
                          VkDeviceSize bufferSize);

  void createShaderStorageBuffers(
      DeviceManager &deviceManager, VkCommandPool &commandPool,
      std::vector<VkBuffer> &shaderStorageBuffers,
      std::vector<VkDeviceMemory> &shaderStorageBuffersMemory,
      glm::vec3 center);

  void createUniformBuffers(VkDevice &device, VkPhysicalDevice &physicalDevice,
                            std::vector<VkBuffer> &uniformBuffers,
                            std::vector<VkDeviceMemory> &uniformBuffersMemory,
                            std::vector<void *> &uniformBuffersMapped,
                            VkDeviceSize bufferSize);

  void createBuffer(VkDevice &device, VkPhysicalDevice &physicalDevice,
                    VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);

  void copyBuffer(VkDevice &device, VkCommandPool &commandPool,
                  VkQueue &graphicsQueue, VkBuffer srcBuffer,
                  VkBuffer dstBuffer, VkDeviceSize size);

  void cleanUp(VkDevice &device) {
    vkDestroyBuffer(device, sphereBuffer, nullptr);
    vkDestroyBuffer(device, quadBuffer, nullptr);
    vkFreeMemory(device, sphereMemory, nullptr);
    vkFreeMemory(device, quadMemory, nullptr);
  }

  VkDeviceMemory &getSphereMemory() { return sphereMemory; }
  VkBuffer &getSphereBuffer() { return sphereBuffer; }
};

#endif // BUFFERMANAGER_H