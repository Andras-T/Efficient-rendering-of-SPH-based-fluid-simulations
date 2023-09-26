#pragma once

#include "Vulkan/BufferManager/BufferManager.h"
#include "Vulkan/CommandPool/CommandPoolManager.h"
#include "Vulkan/DescriptorManager/DescriptorManager.h"
#include "Vulkan/DeviceManager/DeviceManager.h"
#include "Vulkan/FluidInstance.h"
#include "Vulkan/PipelineManager/PipelineManager.h"
#include "Vulkan/Render/Render.h"
#include "Vulkan/SwapchainManager/SwapchainManager.h"
#include "Vulkan/VulkanObject/VulkanObject.h"
#include "Vulkan/Window/Window.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include <imgui_impl_vulkan.h>
#include <iostream>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class FluidSimulationEnvironment {

private:
  Window window;
  VulkanObject vulkanObject;
  DeviceManager deviceManager;
  SwapchainManager swapChainManager;
  BufferManager bufferManager;
  DescriptorManager descriptorManager;
  PipelineManager pipelineManager;
  CommandPoolManager commandPoolManager;
  Render render;

  VkDevice device;
  VkPhysicalDevice physicalDevice;

  FluidInstance instance;

  double lastFrameTime = 0.0f;

  const char *vertPath = "\\shaders\\vert.spv";
  const char *fragPath = "\\shaders\\frag.spv";
  const char *compPath = "\\shaders\\comp.spv";

public:
  static void check_vk_result(VkResult err) {
    if (err == 0)
      return;
    std::cerr << "\x1b[31m"
              << "[vulkan] Error: VkResult = " << err << "\x1b[0m"
              << "\n";
    if (err < 0) {
      std::cerr << "\x1b[31m"
                << "[vulkan] Aborting"
                << "\x1b[0m"
                << "\n";
      abort();
    }
  }

  void init();

  void initImGui();

  void run();

  void mainLoop();

  void cleanUp();
};
