#pragma once

#include "../../Logger.h"
#include "../CommandPool/CommandPoolManager.h"
#include "../Utils/Structs/QueueFamilyIndices.h"
#include "../Utils/Structs/SwapChainSupportDetails.h"
#include "../VulkanObject/VulkanObject.h"
#include "../Window/Window.h"
#include "Image.h"

class SwapchainManager {

private:
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkFramebuffer> quadSwapChainFramebuffers;
  std::vector<VkFramebuffer> blurSwapChainFramebuffers;

  Image depthImage;
  Image blurImage;

  Window *window;
  VkSurfaceKHR *surface;
  VkPhysicalDevice *physicalDevice;
  VkDevice *device;
  SwapChainSupportDetails swapChainSupportDetails;
  QueueFamilyIndices *indices;
  VulkanObject *vulkanObject;

  Logger &logger;

  uint32_t minImageCount;

  void createSwapChain();

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice *phDevice,
                                                VkSurfaceKHR &surface);

  void createImageViews();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);

public:
  SwapchainManager() : logger(Logger::getInstance()) {}

  void init(Window &window, VkSurfaceKHR &surface,
            VkPhysicalDevice &physicalDevice, VkDevice &device,
            QueueFamilyIndices &indices);

  void createDepthResources(CommandPoolManager &commandPoolManager);

  void createFramebuffers(VulkanObject &vulkanObject);

  void recreateSwapChain(GLFWwindow *window, VkDevice &device,
                         CommandPoolManager &commandPoolManager);

  void cleanupSwapChain();

  VkFormat findDepthFormat();

  VkFormat &getSwapChainImageFormat() { return swapChainImageFormat; }
  VkExtent2D &getSwapChainExtent() { return swapChainExtent; }
  VkSwapchainKHR &getSwapChain() { return swapChain; }
  std::vector<VkFramebuffer> &getSwapChainFramebuffers() {
    return swapChainFramebuffers;
  }

  std::vector<VkFramebuffer> &getQuadSwapChainFramebuffers() {
    return quadSwapChainFramebuffers;
  }

  std::vector<VkFramebuffer> &getBlurSwapChainFramebuffers() {
    return blurSwapChainFramebuffers;
  }

  uint32_t getSwapchainImageCount() { return swapChainImages.size(); }

  uint32_t getMinImageCount() { return minImageCount; }

  VkImage &getDepthImage() { return depthImage.getImage(); }
  VkDeviceMemory &getDepthImageMemory() { return depthImage.getImageMemory(); }
  VkImageView &getDepthImageView() { return depthImage.getImageView(); }

  VkImage &getBlurImage() { return blurImage.getImage(); }
  VkDeviceMemory &getBlurImageMemory() { return blurImage.getImageMemory(); }
  VkImageView &getBlurImageView() { return blurImage.getImageView(); }
};
