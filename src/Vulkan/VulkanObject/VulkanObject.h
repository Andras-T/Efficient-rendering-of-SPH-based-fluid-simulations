#pragma once

#define GLFW_INCLUDE_VULKAN

#include "Logger/Logger.h"
#include "GLFW/glfw3.h"
#include <vector>

class VulkanObject {

private:
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkRenderPass simulationRenderPass;
  VkRenderPass quadRenderPass;
  VkRenderPass blurRenderPass;

  Logger &logger;

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  VkDebugUtilsMessengerEXT &getDebugMessenger() { return debugMessenger; }

  void createInstance(bool enableValidationLayers);

  void setupDebugMessenger(bool enableValidationLayers);

  void createSurface(GLFWwindow &window);

  bool checkValidationLayerSupport();

  std::vector<const char *> getRequiredExtensions(bool enableValidationLayers);

  static VkBool32
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);

  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  void DestroyDebugUtilsMessengerEXT();

public:
  VulkanObject() : logger(Logger::getInstance()) {}

  void init(GLFWwindow &window, bool enableValidationLayers);

  void createRenderPass(VkDevice &device, VkFormat &imageformat,
                        VkFormat depthformat);

  void cleanup(bool enableValidationLayers);

  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance &instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);

  VkRenderPass &getSimulationRenderPass() { return simulationRenderPass; }

  VkRenderPass &getQuadRenderPass() { return quadRenderPass; }

  VkRenderPass &getBlurRenderPass() { return blurRenderPass; }

  VkInstance &getInstance() { return instance; }

  VkSurfaceKHR &getSurface() { return surface; }
};
