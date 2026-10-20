#pragma once

#include "render/DebugView.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace sph::ui {

struct FluidGuideStep {
  const char* title;
  const char* text;
  render::DebugView view;
};

inline constexpr std::array FluidGuideSteps{
    FluidGuideStep{"Particles",
                   "The fluid is a cloud of SPH particles. Each carries mass, velocity, and "
                   "density, and the rest of the pipeline is built on this sample set.",
                   render::DebugView::Particles},
    FluidGuideStep{"Neighbors",
                   "A uniform grid hashes particles into cells so each one only searches nearby "
                   "neighbors instead of the whole tank.",
                   render::DebugView::Cell},
    FluidGuideStep{"Density",
                   "Density is a poly6 kernel sum over those neighbors. Packed liquid sits near "
                   "rest density; spray reads much lower.",
                   render::DebugView::Density},
    FluidGuideStep{"Velocity",
                   "Pressure from the Tait EOS and a viscosity Laplacian accelerate the particles. "
                   "This is the SPH force step.",
                   render::DebugView::Velocity},
    FluidGuideStep{"Depth",
                   "Screen-space fluid rendering starts here: each particle is drawn as an "
                   "eye-space sphere and written as depth.",
                   render::DebugView::RawDepth},
    FluidGuideStep{"Smooth",
                   "A bilateral filter smooths the depth surface so the sheet reads as liquid "
                   "without melting the silhouette.",
                   render::DebugView::SmoothDepth},
    FluidGuideStep{"Thickness",
                   "Additive thickness is how much water a view ray travels through. Absorption "
                   "and the green-blue tint come from this.",
                   render::DebugView::Thickness},
    FluidGuideStep{"Normals",
                   "Surface normals come from the smoothed depth gradient, so lighting and "
                   "reflections follow the sheet.",
                   render::DebugView::Normals},
    FluidGuideStep{"Foam",
                   "Spray, foam, and bubbles spawn on crests and ride the surface as a separate "
                   "particle layer.",
                   render::DebugView::Foam},
    FluidGuideStep{"Final",
                   "Refraction, reflection, the glass tank, and the HDRI composite into the "
                   "finished frame.",
                   render::DebugView::Final},
};

[[nodiscard]] constexpr std::uint32_t FluidGuideStepCount() {
  return static_cast<std::uint32_t>(FluidGuideSteps.size());
}

[[nodiscard]] inline const FluidGuideStep& FluidGuideAt(std::uint32_t index) {
  const std::uint32_t last = FluidGuideStepCount() - 1U;
  return FluidGuideSteps.at(std::min(index, last));
}

inline void SelectFluidGuideStep(std::uint32_t& step, bool& active, std::uint32_t index) {
  step = std::min(index, FluidGuideStepCount() - 1U);
  active = true;
}

inline bool ContinueFluidGuide(std::uint32_t& step, bool& active) {
  if (!active) {
    return false;
  }
  if (step + 1U >= FluidGuideStepCount()) {
    active = false;
    return false;
  }
  ++step;
  return true;
}

}  // namespace sph::ui
