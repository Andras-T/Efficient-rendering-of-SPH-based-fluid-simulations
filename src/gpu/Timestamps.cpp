#include "gpu/Timestamps.hpp"

#include "gpu/Context.hpp"

#include <array>

namespace sph::gpu {

namespace {

Unique<VkQueryPool> CreateTimestampPool(VkDevice device, std::uint32_t count) {
  const VkQueryPoolCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
      .queryType = VK_QUERY_TYPE_TIMESTAMP,
      .queryCount = count,
  };
  VkQueryPool pool = VK_NULL_HANDLE;
  Check(vkCreateQueryPool(device, &info, nullptr, &pool), "vkCreateQueryPool");
  return {device, pool, vkDestroyQueryPool};
}

void WriteStamp(VkCommandBuffer commandBuffer, VkQueryPool pool, std::uint32_t slot) {
  vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool, slot);
}

}  // namespace

GpuProfiler::GpuProfiler(const Context& context, std::uint32_t framesInFlight)
    : device_(context.device()),
      periodNs_(context.timestampPeriod()),
      supported_(context.timestampsSupported()) {
  if (!supported_ || framesInFlight == 0) {
    supported_ = false;
    return;
  }
  frames_.resize(framesInFlight);
  for (Frame& frame : frames_) {
    frame.pool = CreateTimestampPool(device_, MaxStamps);
  }
}

void GpuProfiler::beginFrame(VkCommandBuffer commandBuffer, std::uint32_t frameIndex) {
  if (!supported_) {
    return;
  }
  current_ = frameIndex % static_cast<std::uint32_t>(frames_.size());
  Frame& frame = frames_.at(current_);
  frame.stampCount = 0;
  frame.intervalCount = 0;
  frame.pending = false;
  vkCmdResetQueryPool(commandBuffer, frame.pool.get(), 0, MaxStamps);
  WriteStamp(commandBuffer, frame.pool.get(), 0);
  frame.stampCount = 1;
}

void GpuProfiler::mark(VkCommandBuffer commandBuffer, std::string_view name) {
  if (!supported_) {
    return;
  }
  Frame& frame = frames_.at(current_);
  if (frame.stampCount >= MaxStamps) {
    return;
  }
  WriteStamp(commandBuffer, frame.pool.get(), frame.stampCount);
  frame.names.at(frame.intervalCount) = std::string(name);
  ++frame.intervalCount;
  ++frame.stampCount;
}

void GpuProfiler::endFrame(VkCommandBuffer commandBuffer) {
  if (!supported_) {
    return;
  }
  Frame& frame = frames_.at(current_);
  if (frame.intervalCount == 0) {
    mark(commandBuffer, "frame");
  }
  frame.pending = true;
}

void GpuProfiler::resolve(std::uint32_t frameIndex) {
  if (!supported_ || frames_.empty()) {
    return;
  }
  Frame& frame = frames_.at(frameIndex % static_cast<std::uint32_t>(frames_.size()));
  if (!frame.pending || frame.stampCount < 2) {
    return;
  }

  std::array<std::uint64_t, MaxStamps> ticks{};
  Check(vkGetQueryPoolResults(device_, frame.pool.get(), 0, frame.stampCount,
                              frame.stampCount * sizeof(std::uint64_t), ticks.data(),
                              sizeof(std::uint64_t),
                              VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT),
        "vkGetQueryPoolResults");

  intervals_.clear();
  intervals_.reserve(frame.intervalCount);
  for (std::uint32_t i = 0; i < frame.intervalCount; ++i) {
    const auto delta = static_cast<double>(ticks.at(i + 1) - ticks.at(i));
    const double milliseconds = delta * static_cast<double>(periodNs_) * 1.0e-6;
    intervals_.push_back(
        {.name = frame.names.at(i), .milliseconds = static_cast<float>(milliseconds)});
  }
  frame.pending = false;
}

}  // namespace sph::gpu
