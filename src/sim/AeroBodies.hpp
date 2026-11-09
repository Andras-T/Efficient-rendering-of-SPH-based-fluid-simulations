#pragma once

#include "sim/Car.hpp"
#include "sim/WindUnits.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace sph::sim {

// Matches WindSolver: BGK tau, nu = (tau - 1/2) / 3.
inline constexpr float LbmRelaxationTime = 0.60F;
inline constexpr float LbmLatticeViscosity = (LbmRelaxationTime - 0.5F) / 3.0F;

// Hoerner, S. F. (1965). Fluid-Dynamic Drag. Bricktown, NJ. Chapter 3:
// subcritical sphere Cd ≈ 0.47 (roughly 1e3 < Re_D < 2e5); cube face-on Cd ≈ 1.05.
inline constexpr float SphereCdSubcritical = 0.47F;
inline constexpr float CubeCdFaceOn = 1.05F;
// Ahmed, S. R., Ramm, G. & Faltin, G. (1984). SAE 840300. 25° rear slant Cd = 0.230
// at Re_L ≈ 4.3e6.
inline constexpr float AhmedCd25Deg = 0.230F;

// Coarse D3Q19 / 96×24×32 / ~10-cell bodies: lattice Re is O(10^1), not the
// published high-Re figures. Bands cover Hoerner/Ahmed plus low-Re Schiller–Naumann
// with room for bounce-back, blockage, ground effect, and a one-cell shell.
inline constexpr float SphereCdBandMin = 0.55F;
inline constexpr float SphereCdBandMax = 6.5F;
inline constexpr float CubeCdBandMin = 0.70F;
inline constexpr float CubeCdBandMax = 8.0F;
inline constexpr float AhmedCdBandMin = 0.12F;
inline constexpr float AhmedCdBandMax = 8.0F;
inline constexpr float BluffClAbsMax = 3.5F;
inline constexpr float AhmedClAbsMax = 5.0F;

[[nodiscard]] constexpr float LatticeReynolds(float characteristicCells,
                                              float latticeSpeed = DefaultLatticeSpeed) {
  return latticeSpeed * characteristicCells / LbmLatticeViscosity;
}

// Schiller, L. & Naumann, A. (1933). Über die grundlegenden Berechnungen bei der
// Schwerkraftaufbereitung. Z. Ver. Dtsch. Ing. 77, 318–320.
// Sphere Cd ≈ 24/Re (1 + 0.15 Re^0.687) for roughly 0.1 < Re < 800.
[[nodiscard]] float SphereCdSchillerNaumann(float reynolds);

[[nodiscard]] std::vector<MeshVertex> BuildBenchmarkCube();
[[nodiscard]] std::vector<MeshVertex> BuildBenchmarkSphere();
[[nodiscard]] std::vector<MeshVertex> BuildBenchmarkAhmed();

// Scale to ~9–20 cells so blockage stays moderate on the 96×24×32 lattice.
void PlaceBluffBody(std::vector<MeshVertex>& mesh, const CarDomain& domain);

void WriteMeshObj(const std::filesystem::path& path, std::span<const MeshVertex> mesh);

struct BodyOccupancy {
  std::uint32_t solids = 0;
  std::uint32_t extentX = 0;
  std::uint32_t extentY = 0;
  std::uint32_t extentZ = 0;
  std::uint32_t frontalCells = 0;
};

[[nodiscard]] BodyOccupancy MeasureSolidOccupancy(const CarDomain& domain,
                                                  std::span<const std::uint32_t> occupancy);

}  // namespace sph::sim
