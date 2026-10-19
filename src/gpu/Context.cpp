#include "gpu/Context.hpp"

#include "core/Log.hpp"

#include <GLFW/glfw3.h>

#include <format>
#include <span>
#include <stdexcept>

namespace sph::gpu {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT /*types*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT* data,
                                             void* userData) {
  if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
    static_cast<std::atomic<std::uint32_t>*>(userData)->fetch_add(1);
    LogError("[vulkan] {}", data->pMessage);
  } else {
    LogWarning("[vulkan] {}", data->pMessage);
  }
  return VK_FALSE;
}

template <typename T>
T Unwrap(vkb::Result<T> result, std::string_view what) {
  if (!result) {
    throw std::runtime_error(std::format("{}: {}", what, result.error().message()));
  }
  return result.value();
}

}  // namespace

Context::Context(const ContextDesc& desc) {
  LoadVulkan();
  try {
    vkb::InstanceBuilder instanceBuilder(vkGetInstanceProcAddr);
    instanceBuilder.set_app_name("sph-fluid")
        .require_api_version(1, 3, 0)
        .set_headless(desc.window == nullptr);
    if (desc.validation) {
      const auto systemInfo = vkb::SystemInfo::get_system_info(vkGetInstanceProcAddr);
      if (!systemInfo || !systemInfo->validation_layers_available) {
        LogWarning("Vulkan validation layers requested but not installed");
      }
      instanceBuilder.request_validation_layers()
          .set_debug_callback(DebugCallback)
          .set_debug_callback_user_data_pointer(&validationErrors_)
          .set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
    }
    if (desc.window != nullptr) {
      std::uint32_t count = 0;
      const char** extensions = glfwGetRequiredInstanceExtensions(&count);
      instanceBuilder.enable_extensions(count, extensions);
    }
    instance_ = Unwrap(instanceBuilder.build(), "Vulkan instance");
    volkLoadInstanceOnly(instance_.instance);

    if (desc.window != nullptr) {
      Check(glfwCreateWindowSurface(instance_.instance, desc.window, nullptr, &surface_),
            "glfwCreateWindowSurface");
    }

    VkPhysicalDeviceFeatures features{};
    features.shaderInt64 = VK_TRUE;
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.scalarBlockLayout = VK_TRUE;
    features12.timelineSemaphore = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.synchronization2 = VK_TRUE;
    features13.dynamicRendering = VK_TRUE;

    vkb::PhysicalDeviceSelector selector(instance_);
    selector.set_minimum_version(1, 3)
        .set_required_features(features)
        .set_required_features_12(features12)
        .set_required_features_13(features13)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .allow_any_gpu_device_type();
    if (surface_ != VK_NULL_HANDLE) {
      selector.set_surface(surface_);
    }
    const auto physicalDevice = Unwrap(selector.select(), "Vulkan device selection");
    device_ = Unwrap(vkb::DeviceBuilder(physicalDevice).build(), "Vulkan device");
    volkLoadDevice(device_.device);

    queue_ = Unwrap(device_.get_queue(vkb::QueueType::graphics), "graphics queue");
    queueFamily_ = Unwrap(device_.get_queue_index(vkb::QueueType::graphics), "graphics queue");
    if (surface_ != VK_NULL_HANDLE) {
      VkBool32 canPresent = VK_FALSE;
      Check(vkGetPhysicalDeviceSurfaceSupportKHR(device_.physical_device, queueFamily_, surface_,
                                                 &canPresent),
            "vkGetPhysicalDeviceSurfaceSupportKHR");
      if (canPresent == VK_FALSE) {
        throw std::runtime_error("graphics queue cannot present to the window surface");
      }
    }

    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.physicalDevice = device_.physical_device;
    allocatorInfo.device = device_.device;
    allocatorInfo.instance = instance_.instance;
    allocatorInfo.pVulkanFunctions = &functions;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    Check(vmaCreateAllocator(&allocatorInfo, &allocator_), "vmaCreateAllocator");
  } catch (...) {
    destroy();
    throw;
  }
  LogInfo("Vulkan device: {}", deviceName());
}

Context::~Context() { destroy(); }

void Context::waitIdle() const { Check(vkDeviceWaitIdle(device_.device), "vkDeviceWaitIdle"); }

void Context::destroy() {
  if (allocator_ != nullptr) {
    vmaDestroyAllocator(allocator_);
    allocator_ = nullptr;
  }
  if (device_.device != VK_NULL_HANDLE) {
    vkb::destroy_device(device_);
    device_ = {};
  }
  if (surface_ != VK_NULL_HANDLE) {
    vkb::destroy_surface(instance_, surface_);
    surface_ = VK_NULL_HANDLE;
  }
  if (instance_.instance != VK_NULL_HANDLE) {
    vkb::destroy_instance(instance_);
    instance_ = {};
  }
}

}  // namespace sph::gpu
