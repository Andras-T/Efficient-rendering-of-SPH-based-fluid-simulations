#include "DeviceManager.h"
#include "../Utils/Utils.h"
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

void DeviceManager::init(VkInstance &instance, VkSurfaceKHR &surface,
                         bool enableValidationLayers) {
  logger.LogInfo("Setting up device");
  pickPhysicalDevice(instance, surface);
  logger.LogInfo("Physical device picked");
  createLogicalDevice(surface, enableValidationLayers);
  logger.LogInfo("Logical device picked");
}

void DeviceManager::createLogicalDevice(VkSurfaceKHR &surface,
                                        bool enableValidationLayers) {
  indices = findQueueFamilies(&physicalDevice, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceSynchronization2FeaturesKHR syncFeature = {};
  syncFeature.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
  syncFeature.synchronization2 = VK_TRUE;
  VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
  deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceFeatures2.pNext = &syncFeature;
  deviceFeatures2.features.depthBounds = VK_TRUE;
  deviceFeatures2.features.fragmentStoresAndAtomics = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pNext = &deviceFeatures2;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0,
                   &graphicsQueue);
  vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0,
                   &computeQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void DeviceManager::pickPhysicalDevice(VkInstance &instance,
                                       VkSurfaceKHR &surface) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (auto &phDevice : devices) {
    if (isDeviceSuitable(&phDevice, surface)) {
      physicalDevice = phDevice;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

bool DeviceManager::isDeviceSuitable(VkPhysicalDevice *phDevice,
                                     VkSurfaceKHR &surface) {
  QueueFamilyIndices indices = findQueueFamilies(phDevice, surface);

  bool extensionsSupported = checkDeviceExtensionSupport(phDevice);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(phDevice, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool DeviceManager::checkDeviceExtensionSupport(VkPhysicalDevice *device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

QueueFamilyIndices DeviceManager::findQueueFamilies(VkPhysicalDevice *device,
                                                    VkSurfaceKHR &surface) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.graphicsAndComputeFamily = i;
      this->queueFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(*device, i, surface, &presentSupport);

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }
    i++;
  }

  return indices;
}

SwapChainSupportDetails
DeviceManager::querySwapChainSupport(VkPhysicalDevice *phDevice,
                                     VkSurfaceKHR &surface) {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      *phDevice, surface, &swapChainSupportDetails.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(*phDevice, surface, &formatCount,
                                       nullptr);

  if (formatCount != 0) {
    swapChainSupportDetails.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        *phDevice, surface, &formatCount,
        swapChainSupportDetails.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(*phDevice, surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    swapChainSupportDetails.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        *phDevice, surface, &presentModeCount,
        swapChainSupportDetails.presentModes.data());
  }

  return swapChainSupportDetails;
}

void DeviceManager::cleanup() { vkDestroyDevice(device, nullptr); }
