#pragma once

#include <cstdint>
#include <string_view>

namespace sph::render {

enum class DebugView : std::uint8_t {
  Final = 0,
  Particles = 1,
  Density = 2,
  Velocity = 3,
  Cell = 4,
  RawDepth = 5,
  SmoothDepth = 6,
  Thickness = 7,
  Normals = 8,
};

[[nodiscard]] const char* DebugViewName(DebugView view);
[[nodiscard]] DebugView ParseDebugView(std::string_view name);
[[nodiscard]] bool IsParticleView(DebugView view);
[[nodiscard]] bool NeedsFluidPasses(DebugView view);

}  // namespace sph::render
