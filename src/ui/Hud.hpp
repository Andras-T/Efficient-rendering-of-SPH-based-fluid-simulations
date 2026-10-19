#pragma once

#include "gpu/Timestamps.hpp"
#include "gpu/Vulkan.hpp"
#include "render/Camera.hpp"
#include "render/DebugView.hpp"
#include "sim/Scene.hpp"

#include <glm/vec3.hpp>

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
  bool paused = false;
  float timeStep = 0.0F;
  std::uint32_t substeps = 0;
  float restDensity = 0.0F;
  float viscosity = 0.0F;
  render::DebugView view = render::DebugView::Final;
  float exposure = 0.85F;
  glm::vec3 absorption{2.45F, 0.42F, 0.12F};
};

void ApplyCameraInput(render::Camera& camera);

// Returns true when the selected scene preset changes.
bool DrawHud(HudState& state);

}  // namespace sph::ui
