#ifndef SWAPCHAINSUPPORTDETAILS_H
#define SWAPCHAINSUPPORTDETAILS_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vector>

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

#endif // SWAPCHAINSUPPORTDETAILS_H
