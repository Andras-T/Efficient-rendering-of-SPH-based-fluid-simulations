#pragma once

#include "sim/Particle.hpp"
#include "sim/SimParams.hpp"

#include <glm/vec3.hpp>

#include <string_view>
#include <vector>

namespace sph::sim {

enum class ScenePreset : std::uint8_t { Empty, DamBreak, Drop, DoubleDam, DropIn };

inline constexpr glm::vec3 WoodenBlockHalf{0.10F, 0.16F, 0.10F};

struct Scene {
  SimParams params;
  std::vector<Particle> particles;
};

[[nodiscard]] const char* ScenePresetName(ScenePreset preset);
[[nodiscard]] ScenePreset ParseScenePreset(std::string_view name);
[[nodiscard]] Scene BuildScene(ScenePreset preset, const SimParams& params = {});
void CalibrateBoundaryPsi(Scene& scene);
[[nodiscard]] glm::vec3 WoodenBlockStart(ScenePreset preset, const SimParams& params);
void AdvanceWoodenBlock(glm::vec3& center, glm::vec3& velocity, const SimParams& params);

}  // namespace sph::sim
