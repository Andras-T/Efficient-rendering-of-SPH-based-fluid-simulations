#pragma once

#include "gpu/Timestamps.hpp"
#include "gpu/Vulkan.hpp"
#include "render/Camera.hpp"
#include "sim/Scene.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace sph::ui {

struct HudState {
  std::string_view deviceName;
  VkExtent2D extent{};
  render::Camera* camera = nullptr;
  std::span<const gpu::GpuProfiler::Interval> timings;
  sim::ScenePreset preset = sim::ScenePreset::DamBreak;
  std::uint32_t particleCount = 0;
};

void ApplyCameraInput(render::Camera& camera);

// Returns true when the selected scene preset changes.
bool DrawHud(HudState& state);

}  // namespace sph::ui
