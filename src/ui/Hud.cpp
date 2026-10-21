#include "ui/Hud.hpp"

#include "sim/Mesh.hpp"
#include "ui/FluidGuide.hpp"
#include "ui/SmokeGuide.hpp"
#include "ui/WindGuide.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace sph::ui {

namespace {

constexpr float RadiansToDegrees = 180.0F / std::numbers::pi_v<float>;
constexpr float DegreesToRadians = std::numbers::pi_v<float> / 180.0F;
constexpr float OrbitSensitivity = 0.0055F;
constexpr float PanInvertX = -1.0F;
constexpr float TruckSpeed = 0.90F;
constexpr float CursorAccelScale = 1800.0F;
constexpr float CursorAccelMax = 700.0F;
constexpr float CursorPush = 420.0F;

float Degrees(float radians) { return radians * RadiansToDegrees; }

ImU32 CpLegendColor(float cp, float cpMin, float cpMax) {
  const glm::vec3 color = sim::ColorFromCp(cp, cpMin, cpMax);
  return IM_COL32(static_cast<int>(std::lround(color.x * 255.0F)),
                  static_cast<int>(std::lround(color.y * 255.0F)),
                  static_cast<int>(std::lround(color.z * 255.0F)), 255);
}

void DrawCpLegend(float cpMin, float cpMax) {
  const float lo = cpMin < 0.0F ? cpMin : sim::AeroCpMin;
  const float hi = cpMax > 0.0F ? cpMax : sim::AeroCpMax;
  ImGui::TextUnformatted("Surface Cp");
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  constexpr float BarWidth = 16.0F;
  constexpr float BarHeight = 118.0F;
  constexpr int Stops = 24;
  ImDrawList* draw = ImGui::GetWindowDrawList();
  for (int i = 0; i < Stops; ++i) {
    const float t = 1.0F - (static_cast<float>(i) + 0.5F) / static_cast<float>(Stops);
    const float cp = glm::mix(lo, hi, t);
    draw->AddRectFilled(
        {origin.x, origin.y + BarHeight * static_cast<float>(i) / Stops},
        {origin.x + BarWidth, origin.y + BarHeight * static_cast<float>(i + 1) / Stops},
        CpLegendColor(cp, lo, hi));
  }
  draw->AddRect({origin.x, origin.y}, {origin.x + BarWidth, origin.y + BarHeight},
                IM_COL32(20, 20, 24, 255));
  const float textX = origin.x + BarWidth + 8.0F;
  const auto high = std::format("{:+.2f}  high p", static_cast<double>(hi));
  const auto low = std::format("{:+.2f}  suction", static_cast<double>(lo));
  draw->AddText({textX, origin.y - 2.0F}, IM_COL32(220, 80, 70, 255), high.c_str());
  draw->AddText({textX, origin.y + BarHeight * 0.5F - 8.0F}, IM_COL32(210, 210, 214, 255), " 0.0");
  draw->AddText({textX, origin.y + BarHeight - 16.0F}, IM_COL32(90, 130, 230, 255), low.c_str());
  ImGui::Dummy({BarWidth + 118.0F, BarHeight});
}

int PresetIndex(sim::ScenePreset preset) {
  switch (preset) {
    case sim::ScenePreset::Empty:
      return 0;
    case sim::ScenePreset::Drop:
      return 2;
    case sim::ScenePreset::DoubleDam:
      return 3;
    case sim::ScenePreset::DropIn:
      return 4;
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
    case 4:
      return sim::ScenePreset::DropIn;
    default:
      return sim::ScenePreset::DamBreak;
  }
}

glm::vec2 MouseNdc() {
  const ImGuiIO& io = ImGui::GetIO();
  return {(2.0F * io.MousePos.x / io.DisplaySize.x) - 1.0F,
          (2.0F * io.MousePos.y / io.DisplaySize.y) - 1.0F};
}

void StartSelectedSimulation(HudState& state) {
  TryStartSimulation(state.simulation, state.showMenu, state.paused);
}

void DrawSimulationMenu(HudState& state) {
  const ImGuiIO& io = ImGui::GetIO();
  ImDrawList* background = ImGui::GetBackgroundDrawList();
  background->AddRectFilled({0.0F, 0.0F}, io.DisplaySize, IM_COL32(6, 8, 14, 188));

  const ImVec2 center{io.DisplaySize.x * 0.5F, io.DisplaySize.y * 0.5F};
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5F, 0.5F});
  ImGui::SetNextWindowSize({400.0F, 0.0F});
  ImGui::Begin("Choose a simulation", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::TextUnformatted("Pick what to run");
  ImGui::Spacing();

  constexpr std::array Choices{SimulationKind::Fluid, SimulationKind::WindTunnel,
                               SimulationKind::Smoke};
  for (const SimulationKind kind : Choices) {
    const bool ready = SimulationReady(kind);
    ImGui::BeginDisabled(!ready);
    if (ImGui::RadioButton(SimulationKindLabel(kind), state.simulation == kind) && ready) {
      state.simulation = kind;
    }
    if (!ready) {
      ImGui::SameLine();
      ImGui::TextUnformatted("Coming soon");
    }
    ImGui::EndDisabled();
  }

  ImGui::Spacing();
  const bool canPlay = SimulationReady(state.simulation);
  ImGui::BeginDisabled(!canPlay);
  if (ImGui::Button("Play", ImVec2(-1.0F, 36.0F)) && canPlay) {
    StartSelectedSimulation(state);
  }
  ImGui::EndDisabled();
  if (ImGui::Button("Guide", ImVec2(-1.0F, 36.0F))) {
    StartSelectedSimulation(state);
    state.showGuide = true;
    state.guideStep = 0;
    state.paused = false;
  }
  ImGui::TextWrapped("Space starts the selected sim. Each mode has a pipeline guide.");
  ImGui::End();
}

[[nodiscard]] bool GuideIsOpen(const HudState& state) { return state.showGuide; }

void CloseGuide(HudState& state) {
  state.showGuide = false;
  state.view = render::DebugView::Final;
  state.windCar = true;
  state.windTracers = false;
  state.windVolume = true;
  state.smokeVolume = true;
  state.smokeClipY = 0.0F;
}

[[nodiscard]] bool ContinueActiveGuide(HudState& state) {
  if (state.simulation == SimulationKind::WindTunnel) {
    return ContinueWindGuide(state.guideStep, state.showGuide);
  }
  if (state.simulation == SimulationKind::Smoke) {
    return ContinueSmokeGuide(state.guideStep, state.showGuide);
  }
  return ContinueFluidGuide(state.guideStep, state.showGuide);
}

void ApplyHotkeys(HudState& state) {
  const ImGuiIO& io = ImGui::GetIO();
  if (ImGui::IsKeyPressed(ImGuiKey_Space) &&
      (state.showMenu || !io.WantCaptureKeyboard || GuideIsOpen(state))) {
    if (state.showMenu) {
      StartSelectedSimulation(state);
    } else {
      state.paused = !state.paused;
    }
  }
  if (state.showMenu) {
    return;
  }
  if (GuideIsOpen(state)) {
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) ||
        ImGui::IsKeyPressed(ImGuiKey_RightBracket) || ImGui::IsKeyPressed(ImGuiKey_C)) {
      if (!ContinueActiveGuide(state) && !state.showGuide) {
        CloseGuide(state);
      }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket) && state.guideStep > 0) {
      --state.guideStep;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      CloseGuide(state);
    }
    return;
  }
  if (io.WantCaptureKeyboard) {
    return;
  }
  if (state.simulation == SimulationKind::WindTunnel || state.simulation == SimulationKind::Smoke) {
    if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket) || ImGui::IsKeyPressed(ImGuiKey_RightBracket) ||
        ImGui::IsKeyPressed(ImGuiKey_2) || ImGui::IsKeyPressed(ImGuiKey_3)) {
      state.view = state.view == render::DebugView::Velocity ? render::DebugView::Final
                                                             : render::DebugView::Velocity;
    }
    return;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
    state.view = render::PrevDebugView(state.view);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
    state.view = render::NextDebugView(state.view);
  }
  for (int i = 0; i < render::NumericDebugViewKeys; ++i) {
    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i))) {
      state.view = static_cast<render::DebugView>(i);
    }
  }
}

}  // namespace

void ApplyCameraInput(render::Camera& camera, glm::vec3 domainMin, glm::vec3 domainMax) {
  const ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureKeyboard) {
    const float dt = std::max(io.DeltaTime, 1.0e-4F);
    float speed = camera.distance() * TruckSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
      speed *= 2.5F;
    }
    float right = 0.0F;
    float up = 0.0F;
    float forward = 0.0F;
    if (ImGui::IsKeyDown(ImGuiKey_D)) {
      right += speed;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A)) {
      right -= speed;
    }
    if (ImGui::IsKeyDown(ImGuiKey_E)) {
      up += speed;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Q)) {
      up -= speed;
    }
    if (ImGui::IsKeyDown(ImGuiKey_W)) {
      forward += speed;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S)) {
      forward -= speed;
    }
    if (right != 0.0F || up != 0.0F || forward != 0.0F) {
      camera.truck(right, up, forward);
    }
    const float turn = 1.35F * dt;
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
      camera.orbit(turn, 0.0F);
    }
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
      camera.orbit(-turn, 0.0F);
    }
    if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
      camera.orbit(0.0F, turn);
    }
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
      camera.orbit(0.0F, -turn);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
      camera.frame(domainMin, domainMax);
    }
  }

  if (io.WantCaptureMouse) {
    return;
  }
  const bool alt = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
  const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
  const bool lmb = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
  const bool rmb = ImGui::IsMouseDragging(ImGuiMouseButton_Right);
  const bool mmb = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
  if (rmb && !alt) {
    camera.orbit(-io.MouseDelta.x * OrbitSensitivity, -io.MouseDelta.y * OrbitSensitivity);
  }
  if (mmb || (lmb && shift)) {
    camera.pan({io.MouseDelta.x * PanInvertX, io.MouseDelta.y});
  }
  if (alt && rmb) {
    camera.dolly(-io.MouseDelta.y * 0.04F);
  }
  if (io.MouseWheel != 0.0F) {
    camera.dolly(io.MouseWheel);
  }
}

CursorForce SamplePaddle(const render::Camera& camera, glm::vec3 domainMin, glm::vec3 domainMax,
                         bool paused, CursorDrag& drag, float radius, float swirl) {
  const ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse || io.DisplaySize.x <= 0.0F || io.DisplaySize.y <= 0.0F) {
    drag.haveHit = false;
    return {};
  }

  auto hit = camera.hitVolume(MouseNdc(), domainMin, domainMax);
  if (!hit.has_value()) {
    hit = camera.hitPlane(MouseNdc(), 0.5F * (domainMin + domainMax), camera.forward());
  }
  if (!hit.has_value()) {
    drag.haveHit = false;
    return {};
  }

  CursorForce result{.position = *hit, .radius = radius, .visible = true};
  result.grabbing = ImGui::IsMouseDown(ImGuiMouseButton_Left);
  result.stirring = !paused && result.grabbing;
  if (result.grabbing) {
    if (drag.haveHit) {
      result.force = (*hit - drag.lastHit) * CursorAccelScale;
      const float magnitude = glm::length(result.force);
      if (magnitude > CursorAccelMax) {
        result.force *= CursorAccelMax / magnitude;
      }
    }
    if (result.stirring) {
      result.swirl = swirl;
      result.push = CursorPush;
    }
  }
  drag.lastHit = *hit;
  drag.haveHit = result.grabbing;
  return result;
}

namespace {

void DrawFluidGuide(HudState& state) {
  ImGui::SetNextWindowPos({12.0F, 210.0F}, ImGuiCond_Always);
  ImGui::Begin("Fluid guide", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("Step %u / %u", state.guideStep + 1U, FluidGuideStepCount());
  ImGui::Spacing();
  const ImVec4 active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
  for (std::uint32_t i = 0; i < FluidGuideStepCount(); ++i) {
    if (i % 2U != 0U) {
      ImGui::SameLine();
    }
    const bool selected = i == state.guideStep;
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Button, active);
    }
    if (ImGui::Button(FluidGuideAt(i).title, ImVec2(154.0F, 0.0F))) {
      SelectFluidGuideStep(state.guideStep, state.showGuide, i);
    }
    if (selected) {
      ImGui::PopStyleColor();
    }
  }
  const FluidGuideStep& current = FluidGuideAt(state.guideStep);
  state.view = current.view;
  ImGui::Separator();
  ImGui::TextUnformatted(current.title);
  ImGui::TextWrapped("%s", current.text);
  ImGui::Spacing();
  const bool last = state.guideStep + 1U >= FluidGuideStepCount();
  if (ImGui::Button(last ? "Finish" : "Continue", ImVec2(154.0F, 0.0F))) {
    if (!ContinueFluidGuide(state.guideStep, state.showGuide) && !state.showGuide) {
      CloseGuide(state);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Close", ImVec2(154.0F, 0.0F))) {
    CloseGuide(state);
  }
  ImGui::TextUnformatted("Enter or ] continues. Esc closes.");
  ImGui::End();
}

void DrawWindGuide(HudState& state) {
  ImGui::SetNextWindowPos({12.0F, 210.0F}, ImGuiCond_Always);
  ImGui::Begin("Wind guide", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("Step %u / %u", state.guideStep + 1U, WindGuideStepCount());
  ImGui::Spacing();
  const ImVec4 active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
  for (std::uint32_t i = 0; i < WindGuideStepCount(); ++i) {
    if (i % 2U != 0U) {
      ImGui::SameLine();
    }
    const bool selected = i == state.guideStep;
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Button, active);
    }
    if (ImGui::Button(WindGuideAt(i).title, ImVec2(154.0F, 0.0F))) {
      SelectWindGuideStep(state.guideStep, state.showGuide, i);
    }
    if (selected) {
      ImGui::PopStyleColor();
    }
  }
  const WindGuideStep& current = WindGuideAt(state.guideStep);
  state.view = current.view;
  state.windCar = current.showCar;
  state.windTracers = current.showTracers;
  state.windVolume = current.showVolume;
  ImGui::Separator();
  ImGui::TextUnformatted(current.title);
  ImGui::TextWrapped("%s", current.text);
  ImGui::Spacing();
  const bool last = state.guideStep + 1U >= WindGuideStepCount();
  if (ImGui::Button(last ? "Finish" : "Continue", ImVec2(154.0F, 0.0F))) {
    if (!ContinueWindGuide(state.guideStep, state.showGuide) && !state.showGuide) {
      CloseGuide(state);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Close", ImVec2(154.0F, 0.0F))) {
    CloseGuide(state);
  }
  ImGui::TextUnformatted("Enter or ] continues. Esc closes.");
  ImGui::End();
}

void DrawSmokeGuide(HudState& state) {
  ImGui::SetNextWindowPos({12.0F, 210.0F}, ImGuiCond_Always);
  ImGui::Begin("Smoke guide", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("Step %u / %u", state.guideStep + 1U, SmokeGuideStepCount());
  ImGui::Spacing();
  const ImVec4 active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
  for (std::uint32_t i = 0; i < SmokeGuideStepCount(); ++i) {
    if (i % 2U != 0U) {
      ImGui::SameLine();
    }
    const bool selected = i == state.guideStep;
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Button, active);
    }
    if (ImGui::Button(SmokeGuideAt(i).title, ImVec2(154.0F, 0.0F))) {
      SelectSmokeGuideStep(state.guideStep, state.showGuide, i);
    }
    if (selected) {
      ImGui::PopStyleColor();
    }
  }
  const SmokeGuideStep& current = SmokeGuideAt(state.guideStep);
  state.view = current.view;
  state.smokeVolume = current.showVolume;
  state.smokeClipY = current.clipY;
  ImGui::Separator();
  ImGui::TextUnformatted(current.title);
  ImGui::TextWrapped("%s", current.text);
  ImGui::Spacing();
  const bool last = state.guideStep + 1U >= SmokeGuideStepCount();
  if (ImGui::Button(last ? "Finish" : "Continue", ImVec2(154.0F, 0.0F))) {
    if (!ContinueSmokeGuide(state.guideStep, state.showGuide) && !state.showGuide) {
      CloseGuide(state);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Close", ImVec2(154.0F, 0.0F))) {
    CloseGuide(state);
  }
  ImGui::TextUnformatted("Enter or ] continues. Esc closes.");
  ImGui::End();
}

}  // namespace

HudResult DrawHud(HudState& state) {
  ApplyHotkeys(state);
  HudResult result;
  if (state.showMenu) {
    DrawSimulationMenu(state);
    return result;
  }
  const ImGuiIO& io = ImGui::GetIO();

  ImGui::SetNextWindowPos({12.0F, 12.0F}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("%.*s", static_cast<int>(state.deviceName.size()), state.deviceName.data());
  ImGui::Text("%ux%u", state.extent.width, state.extent.height);
  ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(io.Framerate),
              1000.0 / static_cast<double>(io.Framerate));
  if (state.simulation == SimulationKind::WindTunnel) {
    ImGui::TextUnformatted("D3Q19 LBM grid  (not particles)");
  } else if (state.simulation == SimulationKind::Smoke) {
    ImGui::Text("%u cells", state.particleCount);
  } else {
    ImGui::Text("%u particles", state.particleCount);
  }
  if (state.paused) {
    if (ImGui::Button("Play", ImVec2(110.0F, 0.0F))) {
      state.paused = false;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("paused");
  } else {
    if (ImGui::Button("Pause", ImVec2(110.0F, 0.0F))) {
      state.paused = true;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("running");
  }
  ImGui::SameLine();
  if (ImGui::Button("Menu")) {
    state.showMenu = true;
    state.paused = true;
    state.showGuide = false;
  }
  if (state.simulation == SimulationKind::WindTunnel) {
    ImGui::TextUnformatted("Wind tunnel  D3Q19 lattice Boltzmann");
    ImGui::SliderFloat("Inlet speed", &state.windSpeed, 0.03F, 0.12F, "%.3f");
    if (ImGui::Button("Reset")) {
      result.resetScene = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(state.showGuide ? "Exit guide" : "Guide")) {
      state.showGuide = !state.showGuide;
      state.guideStep = state.showGuide ? 0 : state.guideStep;
      if (state.showGuide) {
        state.paused = false;
      } else {
        CloseGuide(state);
      }
    }
    ImGui::TextUnformatted("Space play/pause. [ ] toggles velocity.");
    ImGui::End();

    if (state.showGuide) {
      DrawWindGuide(state);
    } else {
      ImGui::SetNextWindowPos({300.0F, 12.0F}, ImGuiCond_FirstUseEver);
      ImGui::Begin("Stage", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      int windView = 0;
      if (state.view == render::DebugView::Velocity) {
        windView = 1;
      } else if (state.windTracers && !state.windVolume) {
        windView = 2;
      }
      if (ImGui::Combo("View", &windView, "Final\0Velocity\0Tracers\0")) {
        if (windView == 1) {
          state.view = render::DebugView::Velocity;
          state.windVolume = true;
          state.windTracers = false;
        } else if (windView == 2) {
          state.view = render::DebugView::Final;
          state.windVolume = false;
          state.windTracers = true;
        } else {
          state.view = render::DebugView::Final;
          state.windVolume = true;
          state.windTracers = false;
        }
      }
      ImGui::Checkbox("Car", &state.windCar);
      ImGui::SameLine();
      ImGui::Checkbox("Wake", &state.windVolume);
      ImGui::SliderFloat("Exposure", &state.exposure, 0.05F, 4.0F, "%.2f");
      std::array<char, 512> pathBuf{};
      const std::size_t copy = std::min(state.meshPath.size(), pathBuf.size() - 1U);
      std::copy_n(state.meshPath.begin(), copy, pathBuf.begin());
      if (ImGui::InputText("Mesh", pathBuf.data(), pathBuf.size())) {
        state.meshPath = pathBuf.data();
      }
      if (ImGui::Button("Load")) {
        result.loadMesh = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Built-in")) {
        result.useBuiltinCar = true;
      }
      ImGui::TextUnformatted("glTF, glb, OBJ, STL, FBX. Drop a file on the window.");
      if (!state.meshStatus.empty()) {
        ImGui::TextWrapped("%s", state.meshStatus.c_str());
      }
      ImGui::End();
    }
  } else if (state.simulation == SimulationKind::Smoke) {
    ImGui::TextUnformatted("Smoke  Eulerian NS");
    ImGui::SliderFloat("Emitter", &state.smokeStrength, 0.40F, 2.00F, "%.2f");
    if (ImGui::Button("Reset")) {
      result.resetScene = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(state.showGuide ? "Exit guide" : "Guide")) {
      state.showGuide = !state.showGuide;
      state.guideStep = state.showGuide ? 0 : state.guideStep;
      if (state.showGuide) {
        state.paused = false;
      } else {
        CloseGuide(state);
      }
    }
    ImGui::TextUnformatted("Space play/pause. [ ] toggles velocity.");
    ImGui::End();

    if (state.showGuide) {
      DrawSmokeGuide(state);
    } else {
      ImGui::SetNextWindowPos({300.0F, 12.0F}, ImGuiCond_FirstUseEver);
      ImGui::Begin("Stage", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      int smokeView = state.view == render::DebugView::Velocity ? 1 : 0;
      if (ImGui::Combo("View", &smokeView, "Final\0Velocity\0")) {
        state.view = smokeView == 1 ? render::DebugView::Velocity : render::DebugView::Final;
      }
      ImGui::SliderFloat("Exposure", &state.exposure, 0.05F, 4.0F, "%.2f");
      ImGui::End();
    }
  } else {
    ImGui::Text("dt %.4f x %u", static_cast<double>(state.timeStep), state.substeps);
    ImGui::Text("rest density %.1f", static_cast<double>(state.restDensity));
    ImGui::SliderFloat("Viscosity", &state.viscosity, 0.0F, 0.40F, "%.3f");
    int solver = state.solver == sim::SolverMethod::Dfsph ? 1 : 0;
    if (ImGui::Combo("Solver", &solver, "WCSPH\0DFSPH\0")) {
      state.solver = solver == 1 ? sim::SolverMethod::Dfsph : sim::SolverMethod::Wcsph;
    }
    int scene = PresetIndex(state.preset);
    if (ImGui::Combo("Scene", &scene, "Empty\0Dam break\0Drop\0Double dam\0Drop-in\0")) {
      state.preset = PresetFromIndex(scene);
      result.sceneChanged = true;
    }
    if (ImGui::Button("Reset")) {
      result.resetScene = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(state.showGuide ? "Exit guide" : "Guide")) {
      state.showGuide = !state.showGuide;
      state.guideStep = state.showGuide ? 0 : state.guideStep;
      if (state.showGuide) {
        state.paused = false;
      } else {
        state.view = render::DebugView::Final;
      }
    }
    ImGui::SliderFloat("Paddle size", &state.paddleRadius, 0.05F, 0.16F, "%.2f");
    ImGui::SliderFloat("Swirl", &state.paddleSwirl, 0.0F, 16.0F, "%.1f");
    ImGui::Checkbox("Wooden block", &state.showBlock);
    ImGui::SameLine();
    ImGui::Checkbox("Float", &state.floatBlock);
    ImGui::TextUnformatted("LMB paddle; moves the block even while paused");
    ImGui::TextUnformatted("Space play/pause. Scene changes stay paused.");
    ImGui::End();

    if (state.showGuide) {
      DrawFluidGuide(state);
    } else {
      ImGui::SetNextWindowPos({300.0F, 12.0F}, ImGuiCond_FirstUseEver);
      ImGui::Begin("Stage", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      int view = static_cast<int>(state.view);
      if (ImGui::Combo(
              "View", &view,
              "Final\0Particles\0Density\0Velocity\0Cell\0Depth\0Smooth\0Thickness\0Normals\0"
              "Foam\0")) {
        state.view = static_cast<render::DebugView>(view);
      }
      int walls = static_cast<int>(state.walls);
      if (ImGui::Combo("Walls", &walls, "Hidden\0Glass\0Particles\0")) {
        state.walls = static_cast<render::WallDisplay>(walls);
      }
      int tank = static_cast<int>(state.tankShape);
      if (ImGui::Combo("Tank", &tank, "Cube\0Cylinder\0Hexagon\0")) {
        state.tankShape = static_cast<sim::TankShape>(tank);
        result.sceneChanged = true;
      }
      ImGui::SliderFloat("Exposure", &state.exposure, 0.05F, 4.0F, "%.2f");
      ImGui::SliderFloat3("Absorption", &state.absorption.x, 0.0F, 8.0F, "%.2f");
      ImGui::TextUnformatted("[ ] cycles stages (foam is last). 1-9 pick the first nine.");
      ImGui::End();
    }
  }

  if (state.camera != nullptr && !state.showGuide) {
    render::Camera& camera = *state.camera;
    ImGui::SetNextWindowPos({12.0F, 520.0F}, ImGuiCond_FirstUseEver);
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
    ImGui::TextUnformatted("RMB orbit, MMB or Shift+LMB pan");
    ImGui::TextUnformatted("Scroll zoom, WASD/QE move, arrows look, F frame");
    if (ImGui::Button("Reset")) {
      const float aspect = camera.aspect();
      camera.reset();
      camera.setAspect(aspect);
    }
    ImGui::End();
  }

  const float gpuX =
      state.extent.width > 12 ? static_cast<float>(state.extent.width) - 12.0F : 12.0F;
  if (state.simulation == SimulationKind::WindTunnel) {
    ImGui::SetNextWindowPos({gpuX, 150.0F}, ImGuiCond_FirstUseEver, {1.0F, 0.0F});
    ImGui::Begin("Aero", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Drag  %.4f", static_cast<double>(state.windDrag));
    ImGui::Text("Lift  %.4f  %s", static_cast<double>(state.windLift),
                state.windLift < 0.0F ? "downforce" : "lift");
    ImGui::Text("Cd    %.3f", static_cast<double>(state.windCd));
    ImGui::Text("Cl    %.3f", static_cast<double>(state.windCl));
    ImGui::Text("Area  %.3f m^2", static_cast<double>(state.windArea));
    ImGui::TextUnformatted("Momentum exchange on the solid");
    ImGui::Spacing();
    DrawCpLegend(state.windCpMin, state.windCpMax);
    ImGui::End();
  }
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
