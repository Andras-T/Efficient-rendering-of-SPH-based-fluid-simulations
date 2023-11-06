#pragma once

#include "../CommandPool/CommandPoolManager.h"
#include "../DescriptorManager/DescriptorManager.h"
#include "../DeviceManager/DeviceManager.h"
#include "../FluidInstance.h"
#include "../PipelineManager/PipelineManager.h"
#include "../SwapchainManager/SwapchainManager.h"
#include "../Utils/Structs/PushConstants/BlurStageConstant.h"
#include "../Utils/Structs/PushConstants/QuadStageConstant.h"
#include "../VulkanObject/VulkanObject.h"
#include "../Window/Window.h"
#include "ImGuiRender.h"
#include "imgui.h"
#include <vector>

class Render {

private:
  std::vector<VkSemaphore> computeFinishedSemaphores;
  std::vector<VkSemaphore> simulationFinishedSemaphores;
  std::vector<VkSemaphore> blur1FinishedSemaphores;
  std::vector<VkSemaphore> blur2FinishedSemaphores;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;

  std::vector<VkFence> simulationInFlightFences;
  std::vector<VkFence> computeInFlightFences;
  std::vector<VkFence> blur1InFlightFences;
  std::vector<VkFence> blur2InFlightFences;
  std::vector<VkFence> displayInFlightFences;

  BufferManager *bufferManager;
  CommandPoolManager *commandPoolManager;
  DeviceManager *deviceManager;
  DescriptorManager *descriptorManager;
  PipelineManager *pipelineManager;
  SwapchainManager *swapChainManager;
  VulkanObject *vulkanObject;
  Window *window;

  std::vector<VkDescriptorSet> *quadDescriptorSets;

  UniformData uniformData;
  ImGuiRender imGuiRender;

  FluidInstance *instance;

  BlurStage blurPushConstant;
  QuadStage quadPushConstant;
  bool stopped = false;

public:
  size_t currentFrame = 0;

  Render() : imGuiRender(uniformData) {}

  void init(DeviceManager &deviceManager, SwapchainManager &swapChainManager,
            CommandPoolManager &commandPoolManager,
            PipelineManager &pipelineManager, VulkanObject &vulkanObject,
            Window &window, BufferManager &bufferManager,
            FluidInstance &instance, DescriptorManager &descriptorManager);

  void createSyncObjects();

  void drawFrame(uint32_t lastFrameTime);

  void recordComputeCommandBuffer(VkCommandBuffer &commandBuffer);

  void recordCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  void recordQuadCommandBuffer(VkCommandBuffer &commandBuffer,
                               uint32_t imageIndex);

  void recordBlurCommandBuffer(VkCommandBuffer &commandBuffer,
                               uint32_t imageIndex);

  void cleanUp();

  UniformData &getUniformData() { return uniformData; }
};
