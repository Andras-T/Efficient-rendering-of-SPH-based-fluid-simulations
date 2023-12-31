#pragma once

#define GLFW_INCLUDE_VULKAN
#include "Logger/Logger.h"
#include "Vulkan/Utils/Structs/QueueFamilyIndices.h"
#include "Vulkan/Utils/Structs/SwapChainSupportDetails.h"
#include "GLFW/glfw3.h"

class DeviceManager {
  VkPhysicalDevice physicalDevice;
  VkDevice device;

  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue computeQueue;

  QueueFamilyIndices indices;
  SwapChainSupportDetails swapChainSupportDetails;

  uint32_t queueFamily;

  Logger &logger;

  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
      VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME};

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

public:
  DeviceManager() : logger(Logger::getInstance()) {}

  void init(VkInstance &instance, VkSurfaceKHR &surface,
            bool enableValidationLayers);
  void createLogicalDevice(VkSurfaceKHR &surface, bool enableValidationLayers);
  void pickPhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface);
  void cleanup();

#pragma region getterFunctions

  VkDevice &getDevice() { return device; }
  VkPhysicalDevice &getPhysicalDevice() { return physicalDevice; }
  QueueFamilyIndices &getFamilyIndices() { return indices; }
  VkQueue &getGraphicsQueue() { return graphicsQueue; }
  VkQueue &getPresentQueue() { return presentQueue; }
  VkQueue &getComputeQueue() { return computeQueue; }
  uint32_t getQueueFamily() { return queueFamily; }

#pragma endregion

private:
  bool isDeviceSuitable(VkPhysicalDevice *phDevice, VkSurfaceKHR &surface);
  bool checkDeviceExtensionSupport(VkPhysicalDevice *device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice *device,
                                       VkSurfaceKHR &surface);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice *phDevice,
                                                VkSurfaceKHR &surface);
};
