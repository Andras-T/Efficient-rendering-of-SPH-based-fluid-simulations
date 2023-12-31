#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "GLFW/glfw3.h"
#include "glm/vec3.hpp"
#include <vector>

#include "Vulkan/DeviceManager/DeviceManager.h"

class BufferManager {

  bool firstRun = true;
  std::vector<Particle> particles;

  VkBuffer sphereBuffer;
  VkDeviceMemory sphereMemory;

  VkBuffer quadBuffer;
  VkDeviceMemory quadMemory;

  VkBuffer skyBoxBuffer;
  VkDeviceMemory skyBoxMemory;

  VkBuffer blurBuffer;
  VkDeviceMemory blurMemory;

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

  static void createBuffer(VkDevice &device, VkPhysicalDevice &physicalDevice,
                           VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory);

  void copyBuffer(VkDevice &device, VkCommandPool &commandPool,
                  VkQueue &graphicsQueue, VkBuffer srcBuffer,
                  VkBuffer dstBuffer, VkDeviceSize size);

  void resetParticles(DeviceManager &deviceManager, VkCommandPool &commandPool,
                      std::vector<VkBuffer> &shaderStorageBuffers,
                      std::vector<VkDeviceMemory> &shaderStorageBuffersMemory);

  void cleanUp(VkDevice &device) {
    vkDestroyBuffer(device, sphereBuffer, nullptr);
    vkDestroyBuffer(device, quadBuffer, nullptr);
    vkDestroyBuffer(device, blurBuffer, nullptr);
    vkDestroyBuffer(device, skyBoxBuffer, nullptr);

    vkFreeMemory(device, sphereMemory, nullptr);
    vkFreeMemory(device, quadMemory, nullptr);
    vkFreeMemory(device, blurMemory, nullptr);
    vkFreeMemory(device, skyBoxMemory, nullptr);
  }

  VkDeviceMemory &getSphereMemory() { return sphereMemory; }
  VkBuffer &getSphereBuffer() { return sphereBuffer; }
  VkBuffer &getQuadBuffer() { return quadBuffer; }
  VkBuffer &getSkyBoxBuffer() { return skyBoxBuffer; }
  VkBuffer &getBlurBuffer() { return blurBuffer; }
};