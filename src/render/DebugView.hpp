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
  Foam = 9,
};

constexpr auto DebugViewCount = static_cast<std::uint8_t>(DebugView::Foam) + 1;
// Number keys 1-9. Key 10 would be ImGuiKey_A and steal WASD strafe.
constexpr int NumericDebugViewKeys = 9;

enum class WallDisplay : std::uint8_t {
  Hidden = 0,
  Glass = 1,
  Particles = 2,
};

[[nodiscard]] const char* DebugViewName(DebugView view);
[[nodiscard]] DebugView ParseDebugView(std::string_view name);
[[nodiscard]] DebugView NextDebugView(DebugView view);
[[nodiscard]] DebugView PrevDebugView(DebugView view);
[[nodiscard]] bool IsParticleView(DebugView view);
[[nodiscard]] bool NeedsFluidPasses(DebugView view);

}  // namespace sph::render
