#pragma once

#include <algorithm>

namespace sph::sim {

inline constexpr float LatticeSpeedMin = 0.02F;
inline constexpr float LatticeSpeedMax = 0.14F;
inline constexpr float DefaultLatticeSpeed = 0.08F;
// 0.08 lattice -> 50 m/s -> 180 km/h. Lattice ux is a fraction of cs; this scale is the
// user-facing tunnel wind, not a Mach number.
inline constexpr float LatticeToMetersPerSecond = 625.0F;
inline constexpr float MetersPerSecondToKmh = 3.6F;
inline constexpr float WindKmhMin = 50.0F;
inline constexpr float WindKmhMax = 280.0F;
inline constexpr float DefaultWindKmh =
    DefaultLatticeSpeed * LatticeToMetersPerSecond * MetersPerSecondToKmh;

[[nodiscard]] constexpr float LatticeToMs(float latticeSpeed) {
  return latticeSpeed * LatticeToMetersPerSecond;
}

[[nodiscard]] constexpr float LatticeToKmh(float latticeSpeed) {
  return LatticeToMs(latticeSpeed) * MetersPerSecondToKmh;
}

[[nodiscard]] constexpr float KmhToLattice(float kmh) {
  return kmh / (LatticeToMetersPerSecond * MetersPerSecondToKmh);
}

[[nodiscard]] inline float ClampLatticeSpeed(float latticeSpeed) {
  return std::clamp(latticeSpeed, LatticeSpeedMin, LatticeSpeedMax);
}

[[nodiscard]] inline float ClampWindKmh(float kmh) {
  return std::clamp(kmh, WindKmhMin, WindKmhMax);
}

}  // namespace sph::sim
