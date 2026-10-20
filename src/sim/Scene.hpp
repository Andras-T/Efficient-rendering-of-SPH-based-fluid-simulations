#pragma once

#include "sim/Particle.hpp"
#include "sim/SimParams.hpp"

#include <glm/vec3.hpp>

#include <span>
#include <string_view>
#include <vector>

namespace sph::sim {

enum class ScenePreset : std::uint8_t { Empty, DamBreak, Drop, DoubleDam, DropIn };

inline constexpr glm::vec3 WoodenBlockHalf{0.10F, 0.16F, 0.10F};
inline constexpr float WoodenBlockDensity = 420.0F;

[[nodiscard]] constexpr float WoodenBlockVolume() {
  return 8.0F * WoodenBlockHalf.x * WoodenBlockHalf.y * WoodenBlockHalf.z;
}

struct Scene {
  SimParams params;
  std::vector<Particle> particles;
};

[[nodiscard]] const char* ScenePresetName(ScenePreset preset);
[[nodiscard]] ScenePreset ParseScenePreset(std::string_view name);
[[nodiscard]] Scene BuildScene(ScenePreset preset, const SimParams& params = {});
void CalibrateBoundaryPsi(Scene& scene);
[[nodiscard]] glm::vec3 WoodenBlockStart(ScenePreset preset, const SimParams& params);
[[nodiscard]] float ColumnWaterSurface(std::span<const Particle> particles, glm::vec3 min,
                                       glm::vec3 max, const SimParams& params);
void AdvanceWoodenBlock(glm::vec3& center, glm::vec3& velocity, const SimParams& params,
                        float waterSurfaceY = 0.0F, bool buoyant = false);

}  // namespace sph::sim
