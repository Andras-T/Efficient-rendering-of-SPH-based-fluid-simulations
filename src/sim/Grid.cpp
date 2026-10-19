#include "sim/Grid.hpp"

#include <algorithm>
#include <cmath>

namespace sph::sim {

Grid MakeGrid(const SimParams& params) {
  Grid grid;
  grid.cellSize = params.supportRadius();
  grid.origin = params.domainMin;
  const glm::vec3 extent = params.domainMax - params.domainMin;
  const float inv = 1.0F / grid.cellSize;
  grid.size = glm::uvec3{
      std::max(1U, static_cast<std::uint32_t>(std::ceil(extent.x * inv))),
      std::max(1U, static_cast<std::uint32_t>(std::ceil(extent.y * inv))),
      std::max(1U, static_cast<std::uint32_t>(std::ceil(extent.z * inv))),
  };
  grid.cellCount = grid.size.x * grid.size.y * grid.size.z;
  return grid;
}

glm::ivec3 CellCoords(glm::vec3 position, const Grid& grid) {
  const glm::vec3 local = (position - grid.origin) / grid.cellSize;
  glm::ivec3 cell{static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y)),
                  static_cast<int>(std::floor(local.z))};
  cell.x = std::clamp(cell.x, 0, static_cast<int>(grid.size.x) - 1);
  cell.y = std::clamp(cell.y, 0, static_cast<int>(grid.size.y) - 1);
  cell.z = std::clamp(cell.z, 0, static_cast<int>(grid.size.z) - 1);
  return cell;
}

std::uint32_t CellIndex(glm::ivec3 coords, const Grid& grid) {
  return static_cast<std::uint32_t>(coords.x) + static_cast<std::uint32_t>(coords.y) * grid.size.x +
         static_cast<std::uint32_t>(coords.z) * grid.size.x * grid.size.y;
}

std::uint32_t CellIndex(glm::vec3 position, const Grid& grid) {
  return CellIndex(CellCoords(position, grid), grid);
}

void ExclusiveScan(std::span<const std::uint32_t> counts, std::span<std::uint32_t> offsets) {
  std::uint32_t sum = 0;
  const std::uint32_t n = static_cast<std::uint32_t>(std::min(counts.size(), offsets.size()));
  for (std::uint32_t i = 0; i < n; ++i) {
    offsets[i] = sum;
    sum += counts[i];
  }
}

void CountingSort(std::span<const std::uint32_t> cellOfParticle,
                  std::span<const std::uint32_t> offsets, std::span<std::uint32_t> fill,
                  std::span<std::uint32_t> dest) {
  const auto n = static_cast<std::uint32_t>(cellOfParticle.size());
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(offsets.size()) &&
                            i < static_cast<std::uint32_t>(fill.size());
       ++i) {
    fill[i] = offsets[i];
  }
  for (std::uint32_t i = 0; i < n; ++i) {
    const std::uint32_t cell = cellOfParticle[i];
    dest[i] = fill[cell];
    ++fill[cell];
  }
}

}  // namespace sph::sim
