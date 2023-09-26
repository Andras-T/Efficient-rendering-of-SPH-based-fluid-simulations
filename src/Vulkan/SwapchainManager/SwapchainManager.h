#pragma once

#include "../Utils/Structs/QueueFamilyIndices.h"
#include "../Utils/Structs/SwapChainSupportDetails.h"
#include "../Window/Window.h"

class SwapchainManager {

private:
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkFramebuffer> quadSwapChainFramebuffers;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;

  Window *window;
  VkSurfaceKHR *surface;
  VkPhysicalDevice *physicalDevice;
  VkDevice *device;
  SwapChainSupportDetails swapChainSupportDetails;
  QueueFamilyIndices *indices;
  VkRenderPass *renderPass;

  uint32_t minImageCount;

  void createSwapChain();

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice *phDevice,
                                                VkSurfaceKHR &surface);

  void createImage(uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image,
                   VkDeviceMemory &imageMemory);

  VkImageView createImageView(VkImage image, VkFormat format,
                              VkImageAspectFlags aspectFlags);

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
  void init(Window &window, VkSurfaceKHR &surface,
            VkPhysicalDevice &physicalDevice, VkDevice &device,
            QueueFamilyIndices &indices);

  void createDepthResources();

  void createFramebuffers(VkRenderPass &renderPass);

  void createFramebuffers();

  void recreateSwapChain(GLFWwindow *window, VkDevice &device);

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

  uint32_t getSwapchainImageCount() { return swapChainImages.size(); }

  uint32_t getMinImageCount() { return minImageCount; }
};
