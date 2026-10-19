#pragma once

#include "gpu/Buffer.hpp"
#include "sim/Particle.hpp"

#include <cstdint>
#include <span>

namespace sph::sim {

class GpuParticles {
 public:
  explicit GpuParticles(const gpu::Context& context);

  void upload(std::span<const Particle> particles);

  [[nodiscard]] VkDeviceAddress address() const { return buffer_.address(); }
  [[nodiscard]] std::uint32_t count() const { return count_; }

 private:
  const gpu::Context* context_ = nullptr;
  gpu::Buffer buffer_;
  std::uint32_t count_ = 0;
};

}  // namespace sph::sim
