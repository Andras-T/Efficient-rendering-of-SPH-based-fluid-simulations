#pragma once

#define GLFW_INCLUDE_VULKAN
#include "../../Logger.h"
#include "../Utils/Structs/QueueFamilyIndices.h"
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <vector>

class CommandPoolManager {

  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkCommandBuffer> quadCommandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;
  std::vector<VkCommandBuffer> imGuiCommandBuffers;

  Logger &logger;

public:
  CommandPoolManager() : logger(Logger::getInstance()) {}

  void init(VkDevice &device, QueueFamilyIndices &queueFamilyIndices);

  void createCommandBuffers(VkDevice &device);

  void createQuadCommandBuffer(VkDevice &device);

  VkCommandBuffer beginSingleTimeCommands(VkDevice &device);

  void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                             VkQueue &graphicsQueue, VkDevice &device);

  void cleanup(VkDevice &device);

  VkCommandPool &getCommandPool() { return commandPool; }

  std::vector<VkCommandBuffer> &getComputeCommandBuffers() {
    return computeCommandBuffers;
  }

  std::vector<VkCommandBuffer> &getCommandBuffers() { return commandBuffers; }

  std::vector<VkCommandBuffer> &getQuadCommandBuffers() {
    return quadCommandBuffers;
  }

  std::vector<VkCommandBuffer> &getImGuiCommandBuffers() {
    return imGuiCommandBuffers;
  }

private:
  void createCommandPool(VkDevice &device,
                         QueueFamilyIndices &queueFamilyIndices);

  void createComputeCommandBuffers(VkDevice &device);
  void createImGuiCommandBuffers(VkDevice &device);
};
