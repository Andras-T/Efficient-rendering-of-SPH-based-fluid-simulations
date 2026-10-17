#pragma once

#include "gpu/Vulkan.hpp"

#include <span>

namespace sph::gpu {

class Context;

struct BufferDesc {
  VkDeviceSize size = 0;
  VkBufferUsageFlags usage = 0;
  // VMA_ALLOCATION_CREATE_HOST_ACCESS_* | MAPPED for CPU-visible buffers.
  VmaAllocationCreateFlags allocationFlags = 0;
};

class Buffer {
 public:
  Buffer() = default;
  Buffer(const Context& context, const BufferDesc& desc);
  ~Buffer();

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;

  [[nodiscard]] VkBuffer handle() const { return buffer_; }
  [[nodiscard]] VkDeviceSize size() const { return size_; }
  // Zero unless created with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT.
  [[nodiscard]] VkDeviceAddress address() const { return address_; }

  // Only valid for buffers created with VMA_ALLOCATION_CREATE_MAPPED_BIT.
  template <typename T>
  [[nodiscard]] std::span<T> mapped() const {
    return {static_cast<T*>(mapped_), size_ / sizeof(T)};
  }

  // Make host writes visible to the device / device writes visible to the host.
  void flush() const;
  void invalidate() const;

 private:
  void reset();

  VmaAllocator allocator_ = nullptr;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = nullptr;
  VkDeviceSize size_ = 0;
  VkDeviceAddress address_ = 0;
  void* mapped_ = nullptr;
};

}  // namespace sph::gpu
