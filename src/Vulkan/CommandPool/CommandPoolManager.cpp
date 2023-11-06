#include "CommandPoolManager.h"
#include "../Utils/Utils.h"
#include <iostream>

void CommandPoolManager::init(DeviceManager &deviceManager,
                              QueueFamilyIndices &queueFamilyIndices) {
  this->device = &deviceManager.getDevice();
  this->deviceManager = &deviceManager;

  createCommandPool(queueFamilyIndices);
}

void CommandPoolManager::createCommandBuffers() {
  commandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(*device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  createQuadCommandBuffer();
  createBlurCommandBuffer();
  createComputeCommandBuffers();
  createImGuiCommandBuffers();

  logger.LogInfo("Command buffers created");
}

void CommandPoolManager::createQuadCommandBuffer() {
  quadCommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)quadCommandBuffers.size();

  if (vkAllocateCommandBuffers(*device, &allocInfo,
                               quadCommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate quad command buffers!");
  }
}

void CommandPoolManager::createBlurCommandBuffer() {
  blur1CommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  blur2CommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo1{};
  allocInfo1.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo1.commandPool = commandPool;
  allocInfo1.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo1.commandBufferCount = (uint32_t)blur1CommandBuffers.size();

  VkCommandBufferAllocateInfo allocInfo2{};
  allocInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo2.commandPool = commandPool;
  allocInfo2.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo2.commandBufferCount = (uint32_t)blur2CommandBuffers.size();

  if (vkAllocateCommandBuffers(*device, &allocInfo1,
                               blur1CommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate blur command buffers!");
  }

  if (vkAllocateCommandBuffers(*device, &allocInfo2,
                               blur2CommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate blur command buffers!");
  }
}

void CommandPoolManager::createCommandPool(
    QueueFamilyIndices &queueFamilyIndices) {

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex =
      queueFamilyIndices.graphicsAndComputeFamily.value();

  if (vkCreateCommandPool(*device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics command pool!");
  }
}

void CommandPoolManager::createComputeCommandBuffers() {
  computeCommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

  if (vkAllocateCommandBuffers(*device, &allocInfo,
                               computeCommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate compute command buffers!");
  }
}

VkCommandBuffer CommandPoolManager::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void CommandPoolManager::endSingleTimeCommands(VkCommandBuffer commandBuffer,
                                               VkQueue &graphicsQueue) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(*device, commandPool, 1, &commandBuffer);
}

void CommandPoolManager::transitionImageLayout(VkImage image, VkFormat format,
                                               VkImageLayout oldLayout,
                                               VkImageLayout newLayout,
                                               uint32_t layerCount) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layerCount;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer, deviceManager->getGraphicsQueue());
}

void CommandPoolManager::cleanup() {
  vkDestroyCommandPool(*device, commandPool, nullptr);
}

void CommandPoolManager::createImGuiCommandBuffers() {
  imGuiCommandBuffers.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)imGuiCommandBuffers.size();

  if (vkAllocateCommandBuffers(*device, &allocInfo,
                               imGuiCommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}
