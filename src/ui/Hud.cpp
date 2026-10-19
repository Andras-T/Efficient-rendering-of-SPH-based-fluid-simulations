#include "ui/Hud.hpp"

#include <imgui.h>

#include <numbers>

namespace sph::ui {

namespace {

constexpr float RadiansToDegrees = 180.0F / std::numbers::pi_v<float>;
constexpr float DegreesToRadians = std::numbers::pi_v<float> / 180.0F;
constexpr float OrbitSensitivity = 0.005F;
constexpr float PanInvertX = -1.0F;

float Degrees(float radians) { return radians * RadiansToDegrees; }

int PresetIndex(sim::ScenePreset preset) {
  switch (preset) {
    case sim::ScenePreset::Drop:
      return 1;
    case sim::ScenePreset::DoubleDam:
      return 2;
    default:
      return 0;
  }
}

sim::ScenePreset PresetFromIndex(int index) {
  switch (index) {
    case 1:
      return sim::ScenePreset::Drop;
    case 2:
      return sim::ScenePreset::DoubleDam;
    default:
      return sim::ScenePreset::DamBreak;
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

bool DrawHud(HudState& state) {
  const ImGuiIO& io = ImGui::GetIO();
  bool sceneChanged = false;

  ImGui::SetNextWindowPos({12.0F, 12.0F}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Frame", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("%.*s", static_cast<int>(state.deviceName.size()), state.deviceName.data());
  ImGui::Text("%ux%u", state.extent.width, state.extent.height);
  ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(io.Framerate),
              1000.0 / static_cast<double>(io.Framerate));
  ImGui::Text("%u particles", state.particleCount);
  ImGui::Checkbox("Pause", &state.paused);
  ImGui::Text("dt %.4f x %u", static_cast<double>(state.timeStep), state.substeps);
  ImGui::Text("rest density %.1f", static_cast<double>(state.restDensity));
  ImGui::Text("viscosity %.2f", static_cast<double>(state.viscosity));
  int view = static_cast<int>(state.view);
  if (ImGui::Combo(
          "View", &view,
          "Final\0Particles\0Density\0Velocity\0Cell\0Depth\0Smooth\0Thickness\0Normals\0")) {
    state.view = static_cast<render::DebugView>(view);
  }
  ImGui::SliderFloat("Exposure", &state.exposure, 0.05F, 4.0F, "%.2f");
  ImGui::SliderFloat3("Absorption", &state.absorption.x, 0.0F, 8.0F, "%.2f");
  int scene = PresetIndex(state.preset);
  if (ImGui::Combo("Scene", &scene, "Dam break\0Drop\0Double dam\0")) {
    state.preset = PresetFromIndex(scene);
    sceneChanged = true;
  }
  ImGui::End();

  if (state.camera != nullptr) {
    render::Camera& camera = *state.camera;
    ImGui::SetNextWindowPos({12.0F, 240.0F}, ImGuiCond_FirstUseEver);
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

  ImGui::SetNextWindowPos({12.0F, 530.0F}, ImGuiCond_FirstUseEver);
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
  return sceneChanged;
}

}  // namespace sph::ui
