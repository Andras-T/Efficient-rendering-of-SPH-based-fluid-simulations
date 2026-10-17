#define VMA_IMPLEMENTATION
#include "gpu/Vulkan.hpp"

#include <format>
#include <stdexcept>
#include <string>

namespace sph::gpu {

namespace {

std::string ResultName(VkResult result) {
  switch (result) {
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_INCOMPLETE:
      return "VK_INCOMPLETE";
    case VK_SUBOPTIMAL_KHR:
      return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR:
      return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "VK_ERROR_OUT_OF_DATE_KHR";
    default:
      return std::format("VkResult({})", static_cast<int>(result));
  }
}

}  // namespace

void LoadVulkan() {
  if (volkGetInstanceVersion() != 0) {
    return;
  }
  if (volkInitialize() != VK_SUCCESS) {
    throw std::runtime_error("Vulkan loader not found");
  }
}

void Check(VkResult result, std::string_view what) {
  if (result != VK_SUCCESS) {
    throw std::runtime_error(std::format("{} failed: {}", what, ResultName(result)));
  }
}

}  // namespace sph::gpu
