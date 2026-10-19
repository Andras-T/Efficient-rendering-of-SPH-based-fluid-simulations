#pragma once

#include "gpu/Handles.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sph::gpu {

class Context;

// Timestamp queries for a small number of named GPU intervals, one pool per frame in flight.
class GpuProfiler {
 public:
  static constexpr std::uint32_t MaxStamps = 16;

  struct Interval {
    std::string name;
    float milliseconds = 0.0F;
  };

  GpuProfiler(const Context& context, std::uint32_t framesInFlight);

  [[nodiscard]] bool supported() const { return supported_; }

  void beginFrame(VkCommandBuffer commandBuffer, std::uint32_t frameIndex);
  // Closes the interval since the previous stamp (or beginFrame) under `name`.
  void mark(VkCommandBuffer commandBuffer, std::string_view name);
  void endFrame(VkCommandBuffer commandBuffer);

  // Previous submit that used `frameIndex` must have finished.
  void resolve(std::uint32_t frameIndex);

  [[nodiscard]] std::span<const Interval> intervals() const { return intervals_; }

 private:
  struct Frame {
    Unique<VkQueryPool> pool;
    std::array<std::string, MaxStamps> names{};
    std::uint32_t stampCount = 0;
    std::uint32_t intervalCount = 0;
    bool pending = false;
  };

  VkDevice device_ = VK_NULL_HANDLE;
  float periodNs_ = 0.0F;
  bool supported_ = false;
  std::uint32_t current_ = 0;
  std::vector<Frame> frames_;
  std::vector<Interval> intervals_;
};

}  // namespace sph::gpu
