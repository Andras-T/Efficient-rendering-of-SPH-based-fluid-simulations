#include "sim/Grid.hpp"
#include "sim/SimParams.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace sph::sim {
namespace {

TEST(GridTest, CellIndexMatchesCoordinates) {
  SimParams params;
  params.particleSpacing = 0.50F;
  params.domainMin = {0.0F, 0.0F, 0.0F};
  params.domainMax = {2.0F, 1.0F, 1.0F};
  const Grid grid = MakeGrid(params);
  EXPECT_EQ(grid.size.x, 2U);
  EXPECT_EQ(grid.size.y, 1U);
  EXPECT_EQ(grid.size.z, 1U);
  EXPECT_EQ(grid.cellCount, 2U);
  EXPECT_EQ(CellIndex(glm::vec3{0.1F, 0.1F, 0.1F}, grid), 0U);
  EXPECT_EQ(CellIndex(glm::vec3{1.2F, 0.1F, 0.1F}, grid), 1U);
  EXPECT_EQ(CellIndex(glm::vec3{-10.0F, 0.0F, 0.0F}, grid), 0U);
  EXPECT_EQ(CellIndex(glm::vec3{10.0F, 0.0F, 0.0F}, grid), 1U);
}

TEST(GridTest, ExclusiveScanPrefixesCounts) {
  const std::vector<std::uint32_t> counts{3, 1, 0, 4};
  std::vector<std::uint32_t> offsets(counts.size(), 99);
  ExclusiveScan(counts, offsets);
  EXPECT_EQ(offsets, (std::vector<std::uint32_t>{0, 3, 4, 4}));
}

TEST(GridTest, CountingSortWritesDestSlots) {
  const std::vector<std::uint32_t> cellOf{1, 0, 1, 0, 0};
  const std::vector<std::uint32_t> counts{3, 2};
  std::vector<std::uint32_t> offsets(counts.size());
  ExclusiveScan(counts, offsets);
  std::vector<std::uint32_t> fill(counts.size());
  std::vector<std::uint32_t> dest(cellOf.size());
  CountingSort(cellOf, offsets, fill, dest);
  EXPECT_EQ(dest, (std::vector<std::uint32_t>{3, 0, 4, 1, 2}));
}

}  // namespace
}  // namespace sph::sim
