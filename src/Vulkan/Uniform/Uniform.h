#pragma once
#include "Vulkan/Utils/Utils.h"
#include <vector>

class Uniform {

  std::vector<VkBuffer> buffers;
  std::vector<VkDeviceMemory> buffersMemory;
  std::vector<void *> buffersMapped;

  void createBuffer(VkDevice &device, VkPhysicalDevice &physicalDevice,
                    VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);

public:
  Uniform() { throw std::runtime_error("Invalid Uniform creation!"); }

  explicit Uniform(VkDevice &device, VkPhysicalDevice &physicalDevice,
                   VkDeviceSize bufferSize);

  void cleanUp(VkDevice &device);

  std::vector<VkBuffer> &getBuffers() { return buffers; }
  VkBuffer &getBuffer(size_t i) { return buffers[i]; }

  std::vector<VkDeviceMemory> &getBuffersMemory() { return buffersMemory; }

  std::vector<void *> &getBuffersMapped() { return buffersMapped; }
  void *getBufferMapped(size_t i) { return buffersMapped[i]; }
};