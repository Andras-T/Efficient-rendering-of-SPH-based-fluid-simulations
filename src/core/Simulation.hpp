#pragma once

#include <cstdint>
#include <string_view>

namespace sph {

enum class SimulationKind : std::uint8_t { Fluid, WindTunnel, Smoke };

[[nodiscard]] constexpr bool SimulationReady(SimulationKind kind) {
  return kind == SimulationKind::Fluid || kind == SimulationKind::WindTunnel ||
         kind == SimulationKind::Smoke;
}

inline bool TryStartSimulation(SimulationKind kind, bool& showMenu, bool& paused) {
  if (!SimulationReady(kind)) {
    return false;
  }
  showMenu = false;
  paused = false;
  return true;
}

[[nodiscard]] const char* SimulationKindName(SimulationKind kind);
[[nodiscard]] const char* SimulationKindLabel(SimulationKind kind);
[[nodiscard]] SimulationKind ParseSimulationKind(std::string_view name);

}  // namespace sph
