#pragma once

#include "render/DebugView.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace sph::ui {

struct SmokeGuideStep {
  const char* title;
  const char* text;
  render::DebugView view;
  bool showVolume;
  float clipY;
};

inline constexpr std::array SmokeGuideSteps{
    SmokeGuideStep{"Domain",
                   "Smoke lives on a closed Eulerian grid, not SPH particles. Density and "
                   "temperature sit in the cells of this box.",
                   render::DebugView::Final, false, 0.0F},
    SmokeGuideStep{"Emitter",
                   "A floor jet injects density and heat each step. Isolating the lower cells "
                   "shows the source before buoyancy lifts the plume.",
                   render::DebugView::Final, true, 0.62F},
    SmokeGuideStep{"Density",
                   "Semi-Lagrangian advection carries density with the velocity field, then a "
                   "little dissipates so the plume does not fill the box.",
                   render::DebugView::Density, true, 0.0F},
    SmokeGuideStep{"Temperature",
                   "Temperature rides along with density. Hot cells are orange; they drive "
                   "buoyancy so the smoke rises.",
                   render::DebugView::Particles, true, 0.0F},
    SmokeGuideStep{"Velocity",
                   "Forces, vorticity, and a Jacobi pressure solve make the flow. The heatmap is "
                   "speed after that incompressible projection.",
                   render::DebugView::Velocity, true, 0.0F},
    SmokeGuideStep{"Final",
                   "A volume raymarch of density, lit by the temperature albedo, composites over "
                   "the HDRI.",
                   render::DebugView::Final, true, 0.0F},
};

[[nodiscard]] constexpr std::uint32_t SmokeGuideStepCount() {
  return static_cast<std::uint32_t>(SmokeGuideSteps.size());
}

[[nodiscard]] inline const SmokeGuideStep& SmokeGuideAt(std::uint32_t index) {
  const std::uint32_t last = SmokeGuideStepCount() - 1U;
  return SmokeGuideSteps.at(std::min(index, last));
}

inline void SelectSmokeGuideStep(std::uint32_t& step, bool& active, std::uint32_t index) {
  step = std::min(index, SmokeGuideStepCount() - 1U);
  active = true;
}

inline bool ContinueSmokeGuide(std::uint32_t& step, bool& active) {
  if (!active) {
    return false;
  }
  if (step + 1U >= SmokeGuideStepCount()) {
    active = false;
    return false;
  }
  ++step;
  return true;
}

}  // namespace sph::ui
