#ifndef RENDER_H
#define RENDER_H

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
  size_t currentFrame = 0;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkSemaphore> computeFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> computeInFlightFences;

  DeviceManager *deviceManager;
  DescriptorManager *descriptorManager;
  SwapchainManager *swapChainManager;
  CommandPoolManager *commandPoolManager;
  PipelineManager *pipelineManager;
  VulkanObject *vulkanObject;
  Window *window;

  ImGuiRender imGuiRender;

  std::vector<FluidInstance *> instances;

public:
  void init(DeviceManager &deviceManager, SwapchainManager &swapChainManager,
            CommandPoolManager &commandPoolManager,
            PipelineManager &pipelineManager, VulkanObject &vulkanObject,
            Window &window);

  void addInstance(FluidInstance &instance) {
    instances.push_back(&instance);
    imGuiRender.addInstance(instance);
  }

  void removeLastInstance() {
    instances.pop_back();
    imGuiRender.removeLastInstance();
  }

  void createSyncObjects();

  void drawFrame(uint32_t lastFrameTime);

  void recordComputeCommandBuffer(VkCommandBuffer &commandBuffer);

  void recordCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  void cleanUp();
};

#endif // RENDER_H
