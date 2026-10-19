#include "ui/Hud.hpp"

#include <glm/geometric.hpp>
#include <imgui.h>

#include <numbers>

namespace sph::ui {

namespace {

constexpr float RadiansToDegrees = 180.0F / std::numbers::pi_v<float>;
constexpr float DegreesToRadians = std::numbers::pi_v<float> / 180.0F;
constexpr float OrbitSensitivity = 0.005F;
constexpr float PanInvertX = -1.0F;
constexpr float CursorRadius = 0.22F;
constexpr float CursorAccelScale = 400.0F;
constexpr float CursorAccelMax = 80.0F;

float Degrees(float radians) { return radians * RadiansToDegrees; }

int PresetIndex(sim::ScenePreset preset) {
  switch (preset) {
    case sim::ScenePreset::Empty:
      return 0;
    case sim::ScenePreset::Drop:
      return 2;
    case sim::ScenePreset::DoubleDam:
      return 3;
    default:
      return 1;
  }
}

sim::ScenePreset PresetFromIndex(int index) {
  switch (index) {
    case 0:
      return sim::ScenePreset::Empty;
    case 2:
      return sim::ScenePreset::Drop;
    case 3:
      return sim::ScenePreset::DoubleDam;
    default:
      return sim::ScenePreset::DamBreak;
  }
}

glm::vec2 MouseNdc() {
  const ImGuiIO& io = ImGui::GetIO();
  return {(2.0F * io.MousePos.x / io.DisplaySize.x) - 1.0F,
          (2.0F * io.MousePos.y / io.DisplaySize.y) - 1.0F};
}

void ApplyHotkeys(HudState& state) {
  const ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
    state.paused = !state.paused;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
    state.view = render::PrevDebugView(state.view);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
    state.view = render::NextDebugView(state.view);
  }
  for (int i = 0; i < static_cast<int>(render::DebugViewCount); ++i) {
    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i))) {
      state.view = static_cast<render::DebugView>(i);
    }
  }
}

}  // namespace

void ApplyCameraInput(render::Camera& camera) {
  const ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    return;
  }
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
    camera.orbit(-io.MouseDelta.x * OrbitSensitivity, -io.MouseDelta.y * OrbitSensitivity);
  }
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    camera.pan({io.MouseDelta.x * PanInvertX, io.MouseDelta.y});
  }
  if (io.MouseWheel != 0.0F) {
    camera.dolly(io.MouseWheel);
  }
}

CursorForce SampleMouseStir(const render::Camera& camera, bool paused, CursorDrag& drag) {
  const ImGuiIO& io = ImGui::GetIO();
  if (paused || io.WantCaptureMouse || io.DisplaySize.x <= 0.0F || io.DisplaySize.y <= 0.0F ||
      !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    drag.haveHit = false;
    return {};
  }

  const auto hit = camera.hitPlane(MouseNdc(), camera.target(), camera.forward());
  if (!hit.has_value()) {
    drag.haveHit = false;
    return {};
  }

  glm::vec3 force{0.0F};
  if (drag.haveHit) {
    force = (*hit - drag.lastHit) * CursorAccelScale;
    const float magnitude = glm::length(force);
    if (magnitude > CursorAccelMax) {
      force *= CursorAccelMax / magnitude;
    }
  }
  drag.lastHit = *hit;
  drag.haveHit = true;
  return {.position = *hit, .force = force, .radius = CursorRadius};
}

HudResult DrawHud(HudState& state) {
  ApplyHotkeys(state);
  const ImGuiIO& io = ImGui::GetIO();
  HudResult result;

  ImGui::SetNextWindowPos({12.0F, 12.0F}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("%.*s", static_cast<int>(state.deviceName.size()), state.deviceName.data());
  ImGui::Text("%ux%u", state.extent.width, state.extent.height);
  ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(io.Framerate),
              1000.0 / static_cast<double>(io.Framerate));
  ImGui::Text("%u particles", state.particleCount);
  ImGui::Checkbox("Pause", &state.paused);
  ImGui::Text("dt %.4f x %u", static_cast<double>(state.timeStep), state.substeps);
  ImGui::Text("rest density %.1f", static_cast<double>(state.restDensity));
  ImGui::SliderFloat("Viscosity", &state.viscosity, 0.0F, 2.0F, "%.2f");
  int solver = state.solver == sim::SolverMethod::Dfsph ? 1 : 0;
  if (ImGui::Combo("Solver", &solver, "WCSPH\0DFSPH\0")) {
    state.solver = solver == 1 ? sim::SolverMethod::Dfsph : sim::SolverMethod::Wcsph;
  }
  int scene = PresetIndex(state.preset);
  if (ImGui::Combo("Scene", &scene, "Empty\0Dam break\0Drop\0Double dam\0")) {
    state.preset = PresetFromIndex(scene);
    result.sceneChanged = true;
  }
  if (ImGui::Button("Reset")) {
    result.resetScene = true;
  }
  ImGui::TextUnformatted("Space pauses, LMB drag stirs");
  ImGui::End();

  ImGui::SetNextWindowPos({12.0F, 280.0F}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Stage", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  int view = static_cast<int>(state.view);
  if (ImGui::Combo(
          "View", &view,
          "Final\0Particles\0Density\0Velocity\0Cell\0Depth\0Smooth\0Thickness\0Normals\0Foam\0")) {
    state.view = static_cast<render::DebugView>(view);
  }
  ImGui::SliderFloat("Exposure", &state.exposure, 0.05F, 4.0F, "%.2f");
  ImGui::SliderFloat3("Absorption", &state.absorption.x, 0.0F, 8.0F, "%.2f");
  ImGui::TextUnformatted("[ ] or 1-9 to cycle views, including foam");
  ImGui::End();

  if (state.camera != nullptr) {
    render::Camera& camera = *state.camera;
    ImGui::SetNextWindowPos({12.0F, 500.0F}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    float yaw = Degrees(camera.yaw());
    float pitch = Degrees(camera.pitch());
    float distance = camera.distance();
    float fov = Degrees(camera.fovY());
    bool changed = false;
    changed |= ImGui::SliderFloat("Yaw", &yaw, -180.0F, 180.0F, "%.1f deg");
    changed |= ImGui::SliderFloat("Pitch", &pitch, -80.0F, 80.0F, "%.1f deg");
    if (changed) {
      camera.setYawPitch(yaw * DegreesToRadians, pitch * DegreesToRadians);
    }
    if (ImGui::SliderFloat("Distance", &distance, 0.2F, 40.0F, "%.2f")) {
      camera.setDistance(distance);
    }
    if (ImGui::SliderFloat("Field of view", &fov, 20.0F, 90.0F, "%.1f deg")) {
      camera.setFovY(fov * DegreesToRadians);
    }
    const glm::vec3 position = camera.position();
    ImGui::Text("Eye %.2f  %.2f  %.2f", static_cast<double>(position.x),
                static_cast<double>(position.y), static_cast<double>(position.z));
    ImGui::TextUnformatted("RMB orbit, MMB pan, scroll zoom");
    if (ImGui::Button("Reset")) {
      const float aspect = camera.aspect();
      camera.reset();
      camera.setAspect(aspect);
    }
    ImGui::End();
  }

  const float gpuX =
      state.extent.width > 12 ? static_cast<float>(state.extent.width) - 12.0F : 12.0F;
  ImGui::SetNextWindowPos({gpuX, 12.0F}, ImGuiCond_FirstUseEver, {1.0F, 0.0F});
  ImGui::Begin("GPU", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  if (state.timings.empty()) {
    ImGui::TextUnformatted("No timestamp queries this frame");
  } else {
    float total = 0.0F;
    for (const gpu::GpuProfiler::Interval& interval : state.timings) {
      ImGui::Text("%s  %.3f ms", interval.name.c_str(), static_cast<double>(interval.milliseconds));
      total += interval.milliseconds;
    }
    if (state.timings.size() > 1) {
      ImGui::Separator();
      ImGui::Text("total  %.3f ms", static_cast<double>(total));
    }
  }
  ImGui::End();
  return result;
}

}  // namespace sph::ui
