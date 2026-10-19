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

void DrawHud(std::string_view deviceName, VkExtent2D extent, render::Camera& camera,
             std::span<const gpu::GpuProfiler::Interval> timings) {
  const ImGuiIO& io = ImGui::GetIO();

  ImGui::SetNextWindowPos({12.0F, 12.0F}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Frame", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("%.*s", static_cast<int>(deviceName.size()), deviceName.data());
  ImGui::Text("%ux%u", extent.width, extent.height);
  ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(io.Framerate),
              1000.0 / static_cast<double>(io.Framerate));
  ImGui::End();

  ImGui::SetNextWindowPos({12.0F, 110.0F}, ImGuiCond_FirstUseEver);
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

  ImGui::SetNextWindowPos({12.0F, 400.0F}, ImGuiCond_FirstUseEver);
  ImGui::Begin("GPU", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  if (timings.empty()) {
    ImGui::TextUnformatted("No timestamp queries this frame");
  } else {
    float total = 0.0F;
    for (const gpu::GpuProfiler::Interval& interval : timings) {
      ImGui::Text("%s  %.3f ms", interval.name.c_str(), static_cast<double>(interval.milliseconds));
      total += interval.milliseconds;
    }
    if (timings.size() > 1) {
      ImGui::Separator();
      ImGui::Text("total  %.3f ms", static_cast<double>(total));
    }
  }
  ImGui::End();
}

}  // namespace sph::ui
