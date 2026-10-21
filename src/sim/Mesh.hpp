#pragma once

#include "sim/Car.hpp"

#include <filesystem>
#include <span>
#include <vector>

namespace sph::sim {

void FitMeshToTunnel(std::vector<MeshVertex>& mesh, const CarDomain& domain);

[[nodiscard]] std::vector<std::uint32_t> BuildWindOccupancy(const CarDomain& domain,
                                                            std::span<const MeshVertex> mesh);

[[nodiscard]] std::vector<MeshVertex> LoadObstacleMesh(const std::filesystem::path& path);

[[nodiscard]] bool MeshPathLooksSupported(const std::filesystem::path& path);

}  // namespace sph::sim
