#pragma once

#include "core/Simulation.hpp"
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
  bool paused = true;
  float timeStep = 0.0F;
  std::uint32_t substeps = 0;
  float restDensity = 0.0F;
  float viscosity = 0.0F;
  sim::SolverMethod solver = sim::SolverMethod::Wcsph;
  sim::TankShape tankShape = sim::TankShape::Box;
  render::DebugView view = render::DebugView::Final;
  render::WallDisplay walls = render::WallDisplay::Glass;
  float paddleRadius = 0.10F;
  float paddleSwirl = 6.0F;
  bool showBlock = true;
  bool floatBlock = true;
  bool showMenu = true;
  SimulationKind simulation = SimulationKind::Fluid;
  float windSpeed = 0.08F;
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
  float push = 0.0F;
  bool visible = false;
  bool stirring = false;
  bool grabbing = false;
};

void ApplyCameraInput(render::Camera& camera, glm::vec3 domainMin, glm::vec3 domainMax);

// Left-drag grabs the block even while paused. Stirring the fluid requires Play.
[[nodiscard]] CursorForce SamplePaddle(const render::Camera& camera, glm::vec3 domainMin,
                                       glm::vec3 domainMax, bool paused, CursorDrag& drag,
                                       float radius, float swirl);

HudResult DrawHud(HudState& state);

}  // namespace sph::ui
