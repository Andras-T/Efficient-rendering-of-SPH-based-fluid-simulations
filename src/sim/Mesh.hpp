#pragma once

#include "sim/Car.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
#include <span>
#include <vector>

namespace sph::sim {

inline constexpr float AeroCpMin = -1.5F;
inline constexpr float AeroCpMax = 1.0F;

void FitMeshToTunnel(std::vector<MeshVertex>& mesh, const CarDomain& domain);

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
