#pragma once

#include "sim/SimParams.hpp"

#include <glm/ext/vector_uint3.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace sph::sim {

struct Grid {
  glm::uvec3 size{1, 1, 1};
  float cellSize = 1.0F;
  glm::vec3 origin{0.0F};
  std::uint32_t cellCount = 1;
};

[[nodiscard]] Grid MakeGrid(const SimParams& params);

[[nodiscard]] glm::ivec3 CellCoords(glm::vec3 position, const Grid& grid);

[[nodiscard]] std::uint32_t CellIndex(glm::ivec3 coords, const Grid& grid);

[[nodiscard]] std::uint32_t CellIndex(glm::vec3 position, const Grid& grid);

void ExclusiveScan(std::span<const std::uint32_t> counts, std::span<std::uint32_t> offsets);

void CountingSort(std::span<const std::uint32_t> cellOfParticle,
                  std::span<const std::uint32_t> offsets, std::span<std::uint32_t> fill,
                  std::span<std::uint32_t> dest);

}  // namespace sph::sim
