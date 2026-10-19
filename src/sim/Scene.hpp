#pragma once

#include "sim/Particle.hpp"
#include "sim/SimParams.hpp"

#include <string_view>
#include <vector>

namespace sph::sim {

enum class ScenePreset : std::uint8_t { Empty, DamBreak, Drop, DoubleDam };

struct Scene {
  SimParams params;
  std::vector<Particle> particles;
};

[[nodiscard]] const char* ScenePresetName(ScenePreset preset);
[[nodiscard]] ScenePreset ParseScenePreset(std::string_view name);
[[nodiscard]] Scene BuildScene(ScenePreset preset, const SimParams& params = {});

}  // namespace sph::sim
