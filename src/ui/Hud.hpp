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

enum class MouseTool : std::uint8_t { Paddle, Orbit };

struct HudState {
  std::string_view deviceName;
  VkExtent2D extent{};
  render::Camera* camera = nullptr;
  std::span<const gpu::GpuProfiler::Interval> timings;
  sim::ScenePreset preset = sim::ScenePreset::DamBreak;
  std::uint32_t particleCount = 0;
  bool paused = true;
  float timeStep = 0.0F;
  std::uint32_t substeps = 0;
  float restDensity = 0.0F;
  float viscosity = 0.0F;
  sim::SolverMethod solver = sim::SolverMethod::Wcsph;
  render::DebugView view = render::DebugView::Final;
  render::WallDisplay walls = render::WallDisplay::Glass;
  MouseTool tool = MouseTool::Paddle;
  float paddleRadius = 0.22F;
  float paddleSwirl = 24.0F;
  float exposure = 0.85F;
  glm::vec3 absorption{2.45F, 0.42F, 0.12F};
};

struct HudResult {
  bool sceneChanged = false;
  bool resetScene = false;
};

struct CursorDrag {
  bool haveHit = false;
  glm::vec3 lastHit{0.0F};
};

struct CursorForce {
  glm::vec3 position{0.0F};
  glm::vec3 force{0.0F};
  float radius = 0.0F;
  float swirl = 0.0F;
  bool visible = false;
};

void ApplyCameraInput(render::Camera& camera, glm::vec3 domainMin, glm::vec3 domainMax,
                      bool leftDragOrbits);

// Paddle tool: the cursor follows the mouse on the plane through the camera target.
// Left-drag stirs and swirls; the sphere stays visible while the tool is selected.
[[nodiscard]] CursorForce SamplePaddle(const render::Camera& camera, bool paused, CursorDrag& drag,
                                       float radius, float swirl);

HudResult DrawHud(HudState& state);

}  // namespace sph::ui
