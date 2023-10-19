#pragma once

#include "../CommandPool/CommandPoolManager.h"
#include "../DescriptorManager/DescriptorManager.h"
#include "../DeviceManager/DeviceManager.h"
#include "../FluidInstance.h"
#include "../PipelineManager/PipelineManager.h"
#include "../SwapchainManager/SwapchainManager.h"
#include "../VulkanObject/VulkanObject.h"
#include "../Window/Window.h"
#include "ImGuiRender.h"
#include "imgui.h"
#include <vector>

class Render {

private:
  std::vector<VkSemaphore> computeFinishedSemaphores;
  std::vector<VkSemaphore> simulationFinishedSemaphores;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> simulationInFlightFences;
  std::vector<VkFence> displayInFlightFences;
  std::vector<VkFence> computeInFlightFences;

  BufferManager *bufferManager;
  CommandPoolManager *commandPoolManager;
  DeviceManager *deviceManager;
  DescriptorManager *descriptorManager;
  PipelineManager *pipelineManager;
  SwapchainManager *swapChainManager;
  VulkanObject *vulkanObject;
  Window *window;

  std::vector<VkDescriptorSet> *quadDescriptorSets;

  ImGuiRender imGuiRender;

  FluidInstance *instance;

public:
  size_t currentFrame = 0;

  void init(DeviceManager &deviceManager, SwapchainManager &swapChainManager,
            CommandPoolManager &commandPoolManager,
            PipelineManager &pipelineManager, VulkanObject &vulkanObject,
            Window &window, BufferManager &bufferManager,
            FluidInstance &instance, DescriptorManager &descriptorManager);

  void createSyncObjects();

  void drawFrame(uint32_t lastFrameTime);

  void recordComputeCommandBuffer(VkCommandBuffer &commandBuffer);

  void recordCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  void recordQuad(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  void cleanUp();
};
