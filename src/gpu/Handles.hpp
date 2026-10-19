#pragma once

#include "gpu/Vulkan.hpp"

#include <cstdint>
#include <span>
#include <utility>

namespace sph::gpu {

// Move-only owner of a device-level Vulkan handle.
template <typename Handle>
class Unique {
 public:
  using Deleter = void(VKAPI_PTR*)(VkDevice, Handle, const VkAllocationCallbacks*);

  Unique() = default;
  Unique(VkDevice device, Handle handle, Deleter deleter) noexcept
      : device_(device), handle_(handle), deleter_(deleter) {}
  ~Unique() { reset(); }

  Unique(const Unique&) = delete;
  Unique& operator=(const Unique&) = delete;
  Unique(Unique&& other) noexcept
      : device_(other.device_),
        handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
        deleter_(other.deleter_) {}
  Unique& operator=(Unique&& other) noexcept {
    if (this != &other) {
      reset();
      device_ = other.device_;
      handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
      deleter_ = other.deleter_;
    }
    return *this;
  }

  [[nodiscard]] Handle get() const { return handle_; }
  explicit operator bool() const { return handle_ != VK_NULL_HANDLE; }

  void reset() {
    if (handle_ != VK_NULL_HANDLE) {
      deleter_(device_, handle_, nullptr);
      handle_ = VK_NULL_HANDLE;
    }
  }

 private:
  VkDevice device_ = VK_NULL_HANDLE;
  Handle handle_ = VK_NULL_HANDLE;
  Deleter deleter_ = nullptr;
};

Unique<VkFence> CreateFence(VkDevice device, bool signaled);
Unique<VkSemaphore> CreateBinarySemaphore(VkDevice device);
Unique<VkCommandPool> CreateCommandPool(VkDevice device, std::uint32_t queueFamily,
                                        VkCommandPoolCreateFlags flags = 0);
Unique<VkShaderModule> CreateShaderModule(VkDevice device, std::span<const std::uint32_t> spirv);
Unique<VkSampler> CreateSampler(VkDevice device, VkSamplerAddressMode addressU,
                                VkSamplerAddressMode addressV);
Unique<VkDescriptorSetLayout> CreateDescriptorSetLayout(
    VkDevice device, std::span<const VkDescriptorSetLayoutBinding> bindings);
Unique<VkDescriptorPool> CreateDescriptorPool(VkDevice device, std::uint32_t maxSets,
                                              std::span<const VkDescriptorPoolSize> sizes);
VkDescriptorSet AllocateDescriptorSet(VkDevice device, VkDescriptorPool pool,
                                      VkDescriptorSetLayout layout);
void WriteCombinedImage(VkDevice device, VkDescriptorSet set, std::uint32_t binding,
                        VkImageView view, VkSampler sampler);
void WriteStorageImage(VkDevice device, VkDescriptorSet set, std::uint32_t binding,
                       VkImageView view);

}  // namespace sph::gpu
