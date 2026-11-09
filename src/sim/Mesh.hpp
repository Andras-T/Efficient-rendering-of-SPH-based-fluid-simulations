#pragma once

#include "sim/Car.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <filesystem>
#include <span>
#include <vector>

namespace sph::sim {

inline constexpr float AeroCpMin = -1.5F;
inline constexpr float AeroCpMax = 1.0F;

// Yaw about +Y (heading), pitch about +Z (nose up), roll about +X (bank).
struct ObstaclePose {
  float yaw = 0.0F;
  float pitch = 0.0F;
  float roll = 0.0F;
  glm::vec3 translation{0.0F};
  float scale = 1.0F;

  [[nodiscard]] bool identity() const {
    constexpr float Eps = 1.0e-5F;
    return std::abs(yaw) <= Eps && std::abs(pitch) <= Eps && std::abs(roll) <= Eps &&
           std::abs(translation.x) <= Eps && std::abs(translation.y) <= Eps &&
           std::abs(translation.z) <= Eps && std::abs(scale - 1.0F) <= Eps;
  }
};

void FitMeshToTunnel(std::vector<MeshVertex>& mesh, const CarDomain& domain);

// Scale, then yaw/pitch/roll about the AABB center, then translate. Uniform scale only.
void TransformObstacleMesh(std::vector<MeshVertex>& mesh, const ObstaclePose& pose);

// Shrink and/or shift so the AABB stays off the lattice walls.
void ClampMeshToLattice(std::vector<MeshVertex>& mesh, const CarDomain& domain);

void ApplyObstaclePose(std::vector<MeshVertex>& mesh, const ObstaclePose& pose,
                       const CarDomain& domain);

[[nodiscard]] std::vector<std::uint32_t> BuildWindOccupancy(const CarDomain& domain,
                                                            std::span<const MeshVertex> mesh);

[[nodiscard]] std::vector<MeshVertex> LoadObstacleMesh(const std::filesystem::path& path);

[[nodiscard]] std::vector<MeshVertex> LoadDefaultFormulaCar(const CarDomain& domain);

[[nodiscard]] bool MeshPathLooksSupported(const std::filesystem::path& path);

[[nodiscard]] std::filesystem::path DefaultFormulaCarPath();

[[nodiscard]] glm::vec3 ColorFromCp(float cp, float cpMin = AeroCpMin, float cpMax = AeroCpMax);

struct AeroPaint {
  float cpMin = 0.0F;
  float cpMax = 0.0F;
};

AeroPaint ColorMeshByPressure(std::vector<MeshVertex>& mesh, const CarDomain& domain,
                              std::span<const glm::vec4> macros,
                              std::span<const std::uint32_t> occupancy, float inletUx);

}  // namespace sph::sim
