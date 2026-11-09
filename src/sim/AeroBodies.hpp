#pragma once

#include "sim/Car.hpp"
#include "sim/WindUnits.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace sph::sim {

// Matches the interactive WindSolver: BGK tau, nu = (tau - 1/2) / 3.
inline constexpr float LbmRelaxationTime = 0.60F;
inline constexpr float LbmLatticeViscosity = (LbmRelaxationTime - 0.5F) / 3.0F;

// Hoerner, S. F. (1965). Fluid-Dynamic Drag. Bricktown, NJ. Chapter 3:
// subcritical sphere Cd ≈ 0.47 (roughly 1e3 < Re_D < 2e5); cube face-on Cd ≈ 1.05.
inline constexpr float SphereCdSubcritical = 0.47F;
inline constexpr float CubeCdFaceOn = 1.05F;
// Ahmed, S. R., Ramm, G. & Faltin, G. (1984). SAE 840300. 25° rear slant Cd = 0.230
// at Re_L ≈ 4.3e6. Ride height 50 mm on a 288 mm body.
inline constexpr float AhmedCd25Deg = 0.230F;
inline constexpr float AhmedRideHeightRatio = 50.0F / 288.0F;

// Coarse D3Q19 allowance around Schiller–Naumann at the lattice Re_D, not a
// tunnel certificate. Bounce-back and blockage can inflate Cd; they must not
// dump it in a 0.55–6.5 window.
inline constexpr float SphereSnFactorMin = 0.70F;
inline constexpr float SphereSnFactorMax = 1.85F;
inline constexpr float CubeHoernerFactorMax = 3.0F;
inline constexpr float BluffClAbsMax = 2.5F;
inline constexpr float AhmedClAbsMax = 6.5F;

enum class BluffSit : std::uint8_t { FreeStream, Ground };

struct BluffPlacement {
  BluffSit sit = BluffSit::FreeStream;
  float heightCells = 8.0F;
  float lengthCells = 24.0F;
  float widthCells = 10.0F;
  float stationCells = 32.0F;
  float rideHeightOverBody = AhmedRideHeightRatio;
};

[[nodiscard]] constexpr float LatticeViscosity(float tau = LbmRelaxationTime) {
  return (tau - 0.5F) / 3.0F;
}

[[nodiscard]] constexpr float LatticeReynolds(float characteristicCells,
                                              float latticeSpeed = DefaultLatticeSpeed,
                                              float tau = LbmRelaxationTime) {
  return latticeSpeed * characteristicCells / LatticeViscosity(tau);
}

// Stokes drag on a sphere: Cd = 24/Re. Schiller, L. & Naumann, A. (1933).
// Z. Ver. Dtsch. Ing. 77, 318–320 recovers this as Re → 0.
[[nodiscard]] constexpr float SphereCdStokes(float reynolds) {
  return 24.0F / (reynolds > 1.0e-6F ? reynolds : 1.0e-6F);
}

// Sphere Cd ≈ 24/Re (1 + 0.15 Re^0.687) for roughly 0.1 < Re < 800.
[[nodiscard]] float SphereCdSchillerNaumann(float reynolds);

[[nodiscard]] std::vector<MeshVertex> BuildBenchmarkCube();
[[nodiscard]] std::vector<MeshVertex> BuildBenchmarkSphere();
[[nodiscard]] std::vector<MeshVertex> BuildBenchmarkAhmed();

void PlaceBluffBody(std::vector<MeshVertex>& mesh, const CarDomain& domain);
void PlaceBluffBody(std::vector<MeshVertex>& mesh, const CarDomain& domain,
                    const BluffPlacement& placement);

void WriteMeshObj(const std::filesystem::path& path, std::span<const MeshVertex> mesh);

void PaintSolidSphere(const CarDomain& domain, std::span<std::uint32_t> occupancy, glm::vec3 center,
                      float radiusCells);
void PaintSolidCube(const CarDomain& domain, std::span<std::uint32_t> occupancy, glm::vec3 center,
                    float sideCells);

struct BodyOccupancy {
  std::uint32_t solids = 0;
  std::uint32_t extentX = 0;
  std::uint32_t extentY = 0;
  std::uint32_t extentZ = 0;
  std::uint32_t minX = 0;
  std::uint32_t minY = 0;
  std::uint32_t minZ = 0;
  std::uint32_t frontalCells = 0;
};

[[nodiscard]] BodyOccupancy MeasureSolidOccupancy(const CarDomain& domain,
                                                  std::span<const std::uint32_t> occupancy);

[[nodiscard]] constexpr float DiameterCells(const BodyOccupancy& body) {
  return 0.5F * static_cast<float>(body.extentY + body.extentZ);
}

}  // namespace sph::sim
