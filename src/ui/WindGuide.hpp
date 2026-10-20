#pragma once

#include "render/DebugView.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace sph::ui {

struct WindGuideStep {
  const char* title;
  const char* text;
  render::DebugView view;
  bool showCar;
  bool showTracers;
};

inline constexpr std::array WindGuideSteps{
    WindGuideStep{"Tunnel",
                  "Air lives on a D3Q19 lattice inside this closed box. The floor is a bounce-back "
                  "wall; the rest of the domain starts as fluid cells.",
                  render::DebugView::Final, false, false},
    WindGuideStep{"Car",
                  "The formula car is a solid. Lattice cells inside the body use bounce-back, so "
                  "air cannot stream through the mesh.",
                  render::DebugView::Final, true, false},
    WindGuideStep{"Inlet",
                  "The left face injects equilibrium flow at the inlet speed. Heatmap tracers "
                  "show that incoming stream before the body disturbs it.",
                  render::DebugView::Velocity, false, true},
    WindGuideStep{"Velocity",
                  "Collide and stream update D3Q19 populations. Macroscopic speed from those "
                  "moments is the wake around the car.",
                  render::DebugView::Velocity, true, true},
    WindGuideStep{"Tracers",
                  "Rake particles spawn at the inlet and advect with the interpolated velocity "
                  "field, so they sketch streamlines.",
                  render::DebugView::Final, false, true},
    WindGuideStep{"Final",
                  "The shaded car, ground, HDRI, and smoke rakes composite into the finished "
                  "tunnel view.",
                  render::DebugView::Final, true, true},
};

[[nodiscard]] constexpr std::uint32_t WindGuideStepCount() {
  return static_cast<std::uint32_t>(WindGuideSteps.size());
}

[[nodiscard]] inline const WindGuideStep& WindGuideAt(std::uint32_t index) {
  const std::uint32_t last = WindGuideStepCount() - 1U;
  return WindGuideSteps.at(std::min(index, last));
}

inline void SelectWindGuideStep(std::uint32_t& step, bool& active, std::uint32_t index) {
  step = std::min(index, WindGuideStepCount() - 1U);
  active = true;
}

inline bool ContinueWindGuide(std::uint32_t& step, bool& active) {
  if (!active) {
    return false;
  }
  if (step + 1U >= WindGuideStepCount()) {
    active = false;
    return false;
  }
  ++step;
  return true;
}

}  // namespace sph::ui
