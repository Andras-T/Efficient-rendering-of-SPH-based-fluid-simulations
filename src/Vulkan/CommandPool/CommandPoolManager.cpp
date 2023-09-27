#include "CommandPoolManager.h"
#include "../Utils/Utils.h"
#include <iostream>

void CommandPoolManager::init(VkDevice &device,
                              QueueFamilyIndices &queueFamilyIndices) {
  createCommandPool(device, queueFamilyIndices);
}

void CommandPoolManager::createCommandBuffers(VkDevice &device) {
  commandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  createQuadCommandBuffer(device);
  createComputeCommandBuffers(device);
  createImGuiCommandBuffers(device);

  logger.LogInfo("Command buffers created");
}

void CommandPoolManager::createQuadCommandBuffer(VkDevice &device) {
  quadCommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)quadCommandBuffers.size();

  if (vkAllocateCommandBuffers(device, &allocInfo, quadCommandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void CommandPoolManager::createCommandPool(
    VkDevice &device, QueueFamilyIndices &queueFamilyIndices) {

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex =
      queueFamilyIndices.graphicsAndComputeFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics command pool!");
  }
}

void CommandPoolManager::createComputeCommandBuffers(VkDevice &device) {
  computeCommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

  if (vkAllocateCommandBuffers(device, &allocInfo,
                               computeCommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate compute command buffers!");
  }
}

VkCommandBuffer CommandPoolManager::beginSingleTimeCommands(VkDevice &device) {
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

  return commandBuffer;
}

void CommandPoolManager::endSingleTimeCommands(VkCommandBuffer commandBuffer,
                                               VkQueue &graphicsQueue,
                                               VkDevice &device) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void CommandPoolManager::cleanup(VkDevice &device) {
  vkDestroyCommandPool(device, commandPool, nullptr);
}

void CommandPoolManager::createImGuiCommandBuffers(VkDevice &device) {
  imGuiCommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)imGuiCommandBuffers.size();

  if (vkAllocateCommandBuffers(device, &allocInfo,
                               imGuiCommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}
