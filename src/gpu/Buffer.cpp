#include "gpu/Buffer.hpp"

#include "gpu/Context.hpp"

#include <utility>

namespace sph::gpu {

Buffer::Buffer(const Context& context, const BufferDesc& desc)
    : allocator_(context.allocator()), size_(desc.size) {
  const VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = desc.size,
      .usage = desc.usage,
  };
  const VmaAllocationCreateInfo allocationInfo{
      .flags = desc.allocationFlags,
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  VmaAllocationInfo result{};
  Check(vmaCreateBuffer(allocator_, &bufferInfo, &allocationInfo, &buffer_, &allocation_, &result),
        "vmaCreateBuffer");
  mapped_ = result.pMappedData;

  if ((desc.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0) {
    const VkBufferDeviceAddressInfo addressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer_,
    };
    address_ = vkGetBufferDeviceAddress(context.device(), &addressInfo);
  }
}

Buffer::~Buffer() { reset(); }

Buffer::Buffer(Buffer&& other) noexcept
    : allocator_(other.allocator_),
      buffer_(std::exchange(other.buffer_, VK_NULL_HANDLE)),
      allocation_(std::exchange(other.allocation_, nullptr)),
      size_(std::exchange(other.size_, 0)),
      address_(std::exchange(other.address_, 0)),
      mapped_(std::exchange(other.mapped_, nullptr)) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    reset();
    allocator_ = other.allocator_;
    buffer_ = std::exchange(other.buffer_, VK_NULL_HANDLE);
    allocation_ = std::exchange(other.allocation_, nullptr);
    size_ = std::exchange(other.size_, 0);
    address_ = std::exchange(other.address_, 0);
    mapped_ = std::exchange(other.mapped_, nullptr);
  }
  return *this;
}

void Buffer::flush() const {
  Check(vmaFlushAllocation(allocator_, allocation_, 0, VK_WHOLE_SIZE), "vmaFlushAllocation");
}

void Buffer::invalidate() const {
  Check(vmaInvalidateAllocation(allocator_, allocation_, 0, VK_WHOLE_SIZE),
        "vmaInvalidateAllocation");
}

void Buffer::reset() {
  if (buffer_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
    buffer_ = VK_NULL_HANDLE;
    allocation_ = nullptr;
  }
}

}  // namespace sph::gpu
