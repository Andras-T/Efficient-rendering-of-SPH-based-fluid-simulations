#include "sim/WindProbe.hpp"

#include "sim/WindUnits.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace sph::sim {
namespace {

constexpr float Cs2 = 1.0F / 3.0F;
constexpr float VolumeBias = 0.32F;
constexpr int MaxMarchSteps = 256;

[[nodiscard]] bool FluidOccupancy(std::uint32_t occupancy) {
  return occupancy == static_cast<std::uint32_t>(WindCell::Fluid) ||
         occupancy == static_cast<std::uint32_t>(WindCell::Inlet) ||
         occupancy == static_cast<std::uint32_t>(WindCell::Outlet);
}

[[nodiscard]] std::uint32_t CellCount(const CarDomain& domain) {
  return domain.grid.x * domain.grid.y * domain.grid.z;
}

[[nodiscard]] bool InGrid(const CarDomain& domain, int x, int y, int z) {
  return x >= 0 && y >= 0 && z >= 0 && x < static_cast<int>(domain.grid.x) &&
         y < static_cast<int>(domain.grid.y) && z < static_cast<int>(domain.grid.z);
}

[[nodiscard]] std::uint32_t CellIndex(const CarDomain& domain, int x, int y, int z) {
  return static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(y) * domain.grid.x +
         static_cast<std::uint32_t>(z) * domain.grid.x * domain.grid.y;
}

[[nodiscard]] glm::vec3 CellCenter(const CarDomain& domain, int x, int y, int z) {
  return domain.origin + domain.cellSize * (glm::vec3(x, y, z) + glm::vec3(0.5F));
}

[[nodiscard]] glm::ivec3 CellOf(const CarDomain& domain, glm::vec3 point) {
  const glm::vec3 coord = (point - domain.origin) / domain.cellSize;
  const glm::ivec3 cell{static_cast<int>(std::floor(coord.x)),
                        static_cast<int>(std::floor(coord.y)),
                        static_cast<int>(std::floor(coord.z))};
  return glm::clamp(cell, glm::ivec3(0), glm::ivec3(domain.grid) - 1);
}

[[nodiscard]] std::uint32_t OccupancyAt(const CarDomain& domain,
                                        std::span<const std::uint32_t> occupancy, int x, int y,
                                        int z) {
  if (!InGrid(domain, x, y, z) || occupancy.size() < CellCount(domain)) {
    return static_cast<std::uint32_t>(WindCell::Wall);
  }
  return occupancy[CellIndex(domain, x, y, z)];
}

[[nodiscard]] float InletDensity(const CarDomain& domain, std::span<const glm::vec4> macros,
                                 std::span<const std::uint32_t> occupancy) {
  float rhoInf = 0.0F;
  int count = 0;
  const int gy = static_cast<int>(domain.grid.y);
  const int gz = static_cast<int>(domain.grid.z);
  for (int z = 1; z + 1 < gz; ++z) {
    for (int y = 1; y + 1 < gy; ++y) {
      const std::uint32_t index = CellIndex(domain, 1, y, z);
      if (index >= occupancy.size() || index >= macros.size() ||
          !FluidOccupancy(occupancy[index])) {
        continue;
      }
      rhoInf += macros[index].x;
      ++count;
    }
  }
  if (count == 0) {
    return 1.0F;
  }
  return rhoInf / static_cast<float>(count);
}

[[nodiscard]] glm::vec4 SampleMacro(const CarDomain& domain, std::span<const glm::vec4> macros,
                                    std::span<const std::uint32_t> occupancy, glm::vec3 point) {
  glm::vec4 sum{0.0F};
  int count = 0;
  const glm::ivec3 base = CellOf(domain, point);
  for (int radius = 1; radius <= 2 && count == 0; ++radius) {
    for (int dz = -radius; dz <= radius; ++dz) {
      for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
          const int x = base.x + dx;
          const int y = base.y + dy;
          const int z = base.z + dz;
          if (!InGrid(domain, x, y, z)) {
            continue;
          }
          const std::uint32_t index = CellIndex(domain, x, y, z);
          if (index >= occupancy.size() || index >= macros.size() ||
              !FluidOccupancy(occupancy[index])) {
            continue;
          }
          sum += macros[index];
          ++count;
        }
      }
    }
  }
  if (count == 0) {
    return {1.0F, 0.0F, 0.0F, 0.0F};
  }
  return sum / static_cast<float>(count);
}

[[nodiscard]] std::optional<glm::vec3> NeighborFluid(const CarDomain& domain,
                                                     std::span<const std::uint32_t> occupancy,
                                                     int x, int y, int z) {
  constexpr std::array Ox{1, -1, 0, 0, 0, 0};
  constexpr std::array Oy{0, 0, 1, -1, 0, 0};
  constexpr std::array Oz{0, 0, 0, 0, 1, -1};
  for (int i = 0; i < 6; ++i) {
    const int nx = x + Ox.at(static_cast<std::size_t>(i));
    const int ny = y + Oy.at(static_cast<std::size_t>(i));
    const int nz = z + Oz.at(static_cast<std::size_t>(i));
    if (FluidOccupancy(OccupancyAt(domain, occupancy, nx, ny, nz))) {
      return CellCenter(domain, nx, ny, nz);
    }
  }
  return std::nullopt;
}

bool IntersectAabb(glm::vec3 origin, glm::vec3 direction, glm::vec3 min, glm::vec3 max,
                   float& tEnter, float& tExit) {
  tEnter = 0.0F;
  tExit = 1.0e6F;
  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(direction[axis]) < 1.0e-8F) {
      if (origin[axis] < min[axis] || origin[axis] > max[axis]) {
        return false;
      }
      continue;
    }
    float t0 = (min[axis] - origin[axis]) / direction[axis];
    float t1 = (max[axis] - origin[axis]) / direction[axis];
    if (t0 > t1) {
      std::swap(t0, t1);
    }
    tEnter = std::max(tEnter, t0);
    tExit = std::min(tExit, t1);
  }
  return tExit >= tEnter;
}

WindProbe FillProbe(const CarDomain& domain, std::span<const glm::vec4> macros,
                    std::span<const std::uint32_t> occupancy, float inletUx, glm::vec3 point,
                    bool onBody, bool onGround) {
  const glm::vec4 macro = SampleMacro(domain, macros, occupancy, point);
  const float rhoInf = InletDensity(domain, macros, occupancy);
  const float dynamic = std::max(1.5F * inletUx * inletUx, 1.0e-6F);
  const glm::vec3 velocity{macro.y, macro.z, macro.w};
  const float speedLattice = glm::length(velocity);
  const float speedMs = LatticeToMs(speedLattice);
  WindProbe probe;
  probe.hit = true;
  probe.onBody = onBody;
  probe.onGround = onGround;
  probe.position = point;
  probe.velocity = velocity;
  probe.density = macro.x;
  probe.pressure = macro.x * Cs2;
  probe.cp = (macro.x - rhoInf) / dynamic;
  probe.speedLattice = speedLattice;
  probe.speedMs = speedMs;
  probe.speedKmh = speedMs * MetersPerSecondToKmh;
  if (!std::isfinite(probe.cp)) {
    probe.cp = 0.0F;
  }
  if (!std::isfinite(probe.pressure)) {
    probe.pressure = Cs2;
  }
  return probe;
}

}  // namespace

WindProbe SampleWindPoint(const CarDomain& domain, std::span<const glm::vec4> macros,
                          std::span<const std::uint32_t> occupancy, float inletUx,
                          glm::vec3 point) {
  if (macros.size() < CellCount(domain) || occupancy.size() < CellCount(domain)) {
    return {};
  }
  const glm::ivec3 cell = CellOf(domain, point);
  const std::uint32_t occupancyHere = OccupancyAt(domain, occupancy, cell.x, cell.y, cell.z);
  const bool onBody = occupancyHere == static_cast<std::uint32_t>(WindCell::Solid);
  const bool onGround = occupancyHere == static_cast<std::uint32_t>(WindCell::Wall);
  glm::vec3 sample = point;
  if ((onBody || onGround)) {
    if (const auto neighbor = NeighborFluid(domain, occupancy, cell.x, cell.y, cell.z)) {
      sample = *neighbor;
    }
  }
  return FillProbe(domain, macros, occupancy, inletUx, sample, onBody, onGround);
}

WindProbe ProbeWindRay(const CarDomain& domain, std::span<const glm::vec4> macros,
                       std::span<const std::uint32_t> occupancy, float inletUx, glm::vec3 origin,
                       glm::vec3 direction) {
  if (macros.size() < CellCount(domain) || occupancy.size() < CellCount(domain)) {
    return {};
  }
  const float dirLength = glm::length(direction);
  if (dirLength < 1.0e-8F) {
    return {};
  }
  direction /= dirLength;
  const glm::vec3 min = domain.origin;
  const glm::vec3 max = CarDomainMax(domain);
  float tEnter = 0.0F;
  float tExit = 0.0F;
  if (!IntersectAabb(origin, direction, min, max, tEnter, tExit)) {
    return {};
  }

  const glm::vec3 start = origin + direction * (tEnter + domain.cellSize * 1.0e-3F);
  glm::ivec3 cell = CellOf(domain, start);
  const glm::ivec3 step{direction.x >= 0.0F ? 1 : -1, direction.y >= 0.0F ? 1 : -1,
                        direction.z >= 0.0F ? 1 : -1};
  glm::vec3 tMax{1.0e30F};
  glm::vec3 tDelta{1.0e30F};
  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(direction[axis]) < 1.0e-8F) {
      continue;
    }
    const float next = domain.origin[axis] +
                       domain.cellSize * static_cast<float>(cell[axis] + (step[axis] > 0 ? 1 : 0));
    tMax[axis] = (next - origin[axis]) / direction[axis];
    tDelta[axis] = domain.cellSize / std::abs(direction[axis]);
  }

  std::optional<glm::vec3> lastFluid;
  float t = tEnter;
  for (int stepCount = 0; stepCount < MaxMarchSteps && t <= tExit; ++stepCount) {
    if (!InGrid(domain, cell.x, cell.y, cell.z)) {
      break;
    }
    const std::uint32_t occupancyHere = OccupancyAt(domain, occupancy, cell.x, cell.y, cell.z);
    const bool onBody = occupancyHere == static_cast<std::uint32_t>(WindCell::Solid);
    const bool onFloor = occupancyHere == static_cast<std::uint32_t>(WindCell::Wall) && cell.y == 0;
    if (onBody || onFloor) {
      const glm::vec3 hit = origin + direction * t;
      glm::vec3 sample = lastFluid.value_or(hit);
      if (!lastFluid) {
        if (const auto neighbor = NeighborFluid(domain, occupancy, cell.x, cell.y, cell.z)) {
          sample = *neighbor;
        }
      }
      return FillProbe(domain, macros, occupancy, inletUx, sample, onBody, onFloor);
    }
    if (FluidOccupancy(occupancyHere)) {
      lastFluid = CellCenter(domain, cell.x, cell.y, cell.z);
    }

    if (tMax.x <= tMax.y && tMax.x <= tMax.z) {
      t = tMax.x;
      tMax.x += tDelta.x;
      cell.x += step.x;
    } else if (tMax.y <= tMax.z) {
      t = tMax.y;
      tMax.y += tDelta.y;
      cell.y += step.y;
    } else {
      t = tMax.z;
      tMax.z += tDelta.z;
      cell.z += step.z;
    }
  }

  const glm::vec3 volume = origin + direction * (tEnter + (tExit - tEnter) * VolumeBias);
  return FillProbe(domain, macros, occupancy, inletUx, volume, false, false);
}

}  // namespace sph::sim
