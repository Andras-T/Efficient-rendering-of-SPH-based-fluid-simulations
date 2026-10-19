#include "sim/GpuParticles.hpp"

#include "gpu/Context.hpp"

#include <cstring>

namespace sph::sim {

GpuParticles::GpuParticles(const gpu::Context& context) : context_(&context) {}

void GpuParticles::upload(std::span<const Particle> particles) {
  count_ = static_cast<std::uint32_t>(particles.size());
  if (count_ == 0) {
    buffer_ = {};
    return;
  }
  const auto bytes = static_cast<VkDeviceSize>(particles.size_bytes());
  buffer_ = gpu::Buffer(
      *context_,
      {
          .size = bytes,
          .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                             VMA_ALLOCATION_CREATE_MAPPED_BIT,
      });
  std::memcpy(buffer_.mapped<std::uint8_t>().data(), particles.data(), particles.size_bytes());
  buffer_.flush();
}

}  // namespace sph::sim
