#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <vector>

#include "../../Logger.h"
#include "../DeviceManager/DeviceManager.h"
#include "../Utils/Structs/QueueFamilyIndices.h"

class CommandPoolManager {

  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkCommandBuffer> quadCommandBuffers;
  std::vector<VkCommandBuffer> blur1CommandBuffers;
  std::vector<VkCommandBuffer> blur2CommandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;
  std::vector<VkCommandBuffer> imGuiCommandBuffers;

  Logger &logger;

  VkDevice *device;
  DeviceManager *deviceManager;

public:
  CommandPoolManager() : logger(Logger::getInstance()) {}

  void init(DeviceManager &deviceManager,
            QueueFamilyIndices &queueFamilyIndices);

  void createCommandBuffers();

  void createQuadCommandBuffer();

  void createBlurCommandBuffer();

  VkCommandBuffer beginSingleTimeCommands();

  void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                             VkQueue &graphicsQueue);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);

  void cleanup();

  VkCommandPool &getCommandPool() { return commandPool; }

  std::vector<VkCommandBuffer> &getComputeCommandBuffers() {
    return computeCommandBuffers;
  }

  std::vector<VkCommandBuffer> &getCommandBuffers() { return commandBuffers; }

  std::vector<VkCommandBuffer> &getQuadCommandBuffers() {
    return quadCommandBuffers;
  }

  std::vector<VkCommandBuffer> &getBlur1CommandBuffers() {
    return blur1CommandBuffers;
  }

  std::vector<VkCommandBuffer>& getBlur2CommandBuffers() {
      return blur2CommandBuffers;
  }

  std::vector<VkCommandBuffer> &getImGuiCommandBuffers() {
    return imGuiCommandBuffers;
  }

private:
  void createCommandPool(QueueFamilyIndices &queueFamilyIndices);

  void createComputeCommandBuffers();
  void createImGuiCommandBuffers();
};
