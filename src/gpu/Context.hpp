#pragma once

#include "gpu/Vulkan.hpp"

#include <VkBootstrap.h>

#include <atomic>
#include <cstdint>
#include <string_view>

struct GLFWwindow;

namespace sph::gpu {

struct ContextDesc {
  bool validation = false;
  // Null creates a headless context without a surface.
  GLFWwindow* window = nullptr;
};

// Instance, device, the single graphics+compute queue, and the memory allocator.
class Context {
 public:
  explicit Context(const ContextDesc& desc);
  ~Context();

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Context(Context&&) = delete;
  Context& operator=(Context&&) = delete;

  [[nodiscard]] VkInstance instance() const { return instance_.instance; }
  [[nodiscard]] VkPhysicalDevice physicalDevice() const { return device_.physical_device; }
  [[nodiscard]] VkDevice device() const { return device_.device; }
  [[nodiscard]] const vkb::Device& bootstrapDevice() const { return device_; }
  [[nodiscard]] VkSurfaceKHR surface() const { return surface_; }
  [[nodiscard]] VkQueue queue() const { return queue_; }
  [[nodiscard]] std::uint32_t queueFamily() const { return queueFamily_; }
  [[nodiscard]] VmaAllocator allocator() const { return allocator_; }
  [[nodiscard]] std::string_view deviceName() const { return device_.physical_device.name; }
  [[nodiscard]] float timestampPeriod() const { return timestampPeriod_; }
  [[nodiscard]] bool timestampsSupported() const { return timestampsSupported_; }
  [[nodiscard]] std::uint32_t validationErrorCount() const { return validationErrors_.load(); }

  void waitIdle() const;

 private:
  void destroy();

  vkb::Instance instance_;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  vkb::Device device_;
  VkQueue queue_ = VK_NULL_HANDLE;
  std::uint32_t queueFamily_ = 0;
  VmaAllocator allocator_ = nullptr;
  float timestampPeriod_ = 0.0F;
  bool timestampsSupported_ = false;
  std::atomic<std::uint32_t> validationErrors_{0};
};

}  // namespace sph::gpu
