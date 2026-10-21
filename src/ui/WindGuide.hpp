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
  bool showVolume;
};

inline constexpr std::array WindGuideSteps{
    WindGuideStep{"Tunnel",
                  "Air lives on a D3Q19 lattice inside this closed box. The floor is a bounce-back "
                  "wall; the rest of the domain starts as fluid cells.",
                  render::DebugView::Final, false, false, false},
    WindGuideStep{"Car",
                  "The default body is the Alfa Romeo C42 (2022 F1 CAD, WTFPL): multi-element "
                  "front wing, floor and diffuser, sidepods, wishbones, beam wing, and 18-inch "
                  "wheels. Cells inside the mesh use bounce-back. The surface is colored by "
                  "pressure coefficient, not paint.",
                  render::DebugView::Final, true, false, false},
    WindGuideStep{"Inlet",
                  "The left face injects equilibrium flow at the inlet speed. A volume heatmap of "
                  "the lattice velocity shows that stream before the body disturbs it.",
                  render::DebugView::Velocity, false, false, true},
    WindGuideStep{"Velocity",
                  "Collide and stream update D3Q19 populations. Macroscopic speed from those "
                  "moments is the wake field around the car.",
                  render::DebugView::Velocity, true, false, true},
    WindGuideStep{"Tracers",
                  "Optional rake particles spawn at the inlet and advect with the interpolated "
                  "velocity field. They sketch streamlines; they are not the solver.",
                  render::DebugView::Final, true, true, false},
    WindGuideStep{"Final",
                  "Cp on the body (blue suction, red high pressure), ground, HDRI, and a volume "
                  "raymarch of the lattice wake. This is D3Q19 LBM, not a particle spray.",
                  render::DebugView::Final, true, false, true},
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
