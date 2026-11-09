#include "sim/AeroBodies.hpp"
#include "sim/Mesh.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

namespace sph::sim {
namespace {

struct Box {
  glm::vec3 min{0.0F};
  glm::vec3 max{0.0F};
};

Box Bounds(const std::vector<MeshVertex>& mesh) {
  Box box{mesh.front().position, mesh.front().position};
  for (const MeshVertex& vertex : mesh) {
    box.min = glm::min(box.min, vertex.position);
    box.max = glm::max(box.max, vertex.position);
  }
  return box;
}

glm::vec3 Extent(const Box& box) { return box.max - box.min; }

std::uint32_t SolidCount(const std::vector<std::uint32_t>& occupancy) {
  std::uint32_t solids = 0;
  for (const std::uint32_t cell : occupancy) {
    solids += cell == static_cast<std::uint32_t>(WindCell::Solid) ? 1U : 0U;
  }
  return solids;
}

std::vector<MeshVertex> LongBoxInTunnel(const CarDomain& domain) {
  auto mesh = BuildBenchmarkCube();
  const glm::vec3 mid = domain.origin + 0.5F * domain.cellSize * glm::vec3(domain.grid);
  for (MeshVertex& vertex : mesh) {
    vertex.position.x *= 0.80F;
    vertex.position.y *= 0.20F;
    vertex.position.z *= 0.24F;
    vertex.position += glm::vec3(mid.x, domain.origin.y + 0.28F, mid.z);
  }
  return mesh;
}

TEST(ObstaclePoseTest, IdentityLeavesTheMeshAlone) {
  const CarDomain domain;
  auto mesh = LongBoxInTunnel(domain);
  const glm::vec3 position = mesh.front().position;
  const glm::vec3 normal = mesh.front().normal;
  ApplyObstaclePose(mesh, {}, domain);
  EXPECT_NEAR(mesh.front().position.x, position.x, 1.0e-6F);
  EXPECT_NEAR(mesh.front().position.y, position.y, 1.0e-6F);
  EXPECT_NEAR(mesh.front().position.z, position.z, 1.0e-6F);
  EXPECT_NEAR(mesh.front().normal.x, normal.x, 1.0e-6F);
  EXPECT_NEAR(mesh.front().normal.y, normal.y, 1.0e-6F);
  EXPECT_NEAR(mesh.front().normal.z, normal.z, 1.0e-6F);
}

TEST(ObstaclePoseTest, Yaw90SwapsLengthAndWidth) {
  const CarDomain domain;
  const auto rest = LongBoxInTunnel(domain);
  auto yawed = rest;
  ApplyObstaclePose(yawed, {.yaw = 0.5F * std::numbers::pi_v<float>}, domain);
  const glm::vec3 restExtent = Extent(Bounds(rest));
  const glm::vec3 yawExtent = Extent(Bounds(yawed));
  EXPECT_NEAR(yawExtent.x, restExtent.z, 0.02F);
  EXPECT_NEAR(yawExtent.z, restExtent.x, 0.02F);
  EXPECT_NEAR(yawExtent.y, restExtent.y, 0.02F);

  std::uint32_t spun = 0;
  for (std::size_t i = 0; i < rest.size(); ++i) {
    if (glm::length(rest[i].normal - glm::vec3(1.0F, 0.0F, 0.0F)) < 0.05F) {
      spun += glm::length(yawed[i].normal - glm::vec3(0.0F, 0.0F, -1.0F)) < 0.05F ? 1U : 0U;
    }
  }
  EXPECT_GE(spun, 6U);
}

TEST(ObstaclePoseTest, Yaw90OccupancyStaysSolid) {
  const CarDomain domain;
  const auto rest = LongBoxInTunnel(domain);
  auto yawed = rest;
  ApplyObstaclePose(yawed, {.yaw = 0.5F * std::numbers::pi_v<float>}, domain);
  const auto before = BuildWindOccupancy(domain, rest);
  const auto after = BuildWindOccupancy(domain, yawed);
  const std::uint32_t solidsBefore = SolidCount(before);
  const std::uint32_t solidsAfter = SolidCount(after);
  EXPECT_GT(solidsBefore, 40U);
  EXPECT_GT(solidsAfter, 40U);
  EXPECT_GT(solidsAfter, solidsBefore / 3U);

  const glm::vec3 dmax = CarDomainMax(domain);
  const Box box = Bounds(yawed);
  EXPECT_GT(box.min.x, domain.origin.x + domain.cellSize);
  EXPECT_LT(box.max.x, dmax.x - domain.cellSize);
  EXPECT_GT(box.min.y, domain.origin.y);
  EXPECT_LT(box.max.y, dmax.y);
  EXPECT_GT(box.min.z, domain.origin.z + domain.cellSize);
  EXPECT_LT(box.max.z, dmax.z - domain.cellSize);
}

TEST(ObstaclePoseTest, PositivePitchRaisesTheNose) {
  const CarDomain domain;
  auto mesh = LongBoxInTunnel(domain);
  std::size_t nose = 0;
  for (std::size_t i = 1; i < mesh.size(); ++i) {
    if (mesh[i].position.x < mesh[nose].position.x) {
      nose = i;
    }
  }
  const float y0 = mesh[nose].position.y;
  ApplyObstaclePose(mesh, {.pitch = 20.0F * std::numbers::pi_v<float> / 180.0F}, domain);
  EXPECT_GT(mesh[nose].position.y, y0 + 0.02F);
}

TEST(ObstaclePoseTest, YawClampKeepsALongBodyInside) {
  const CarDomain domain;
  auto mesh = BuildBenchmarkCube();
  const glm::vec3 mid = domain.origin + 0.5F * domain.cellSize * glm::vec3(domain.grid);
  for (MeshVertex& vertex : mesh) {
    vertex.position.x *= 2.40F;
    vertex.position.y *= 0.12F;
    vertex.position.z *= 0.12F;
    vertex.position += glm::vec3(mid.x, domain.origin.y + 0.30F, mid.z);
  }
  ApplyObstaclePose(mesh, {.yaw = 0.5F * std::numbers::pi_v<float>}, domain);
  const Box box = Bounds(mesh);
  const glm::vec3 dmax = CarDomainMax(domain);
  EXPECT_GT(box.min.x, domain.origin.x + domain.cellSize);
  EXPECT_LT(box.max.x, dmax.x - domain.cellSize);
  EXPECT_GT(box.min.z, domain.origin.z + domain.cellSize);
  EXPECT_LT(box.max.z, dmax.z - domain.cellSize);
  EXPECT_LT(box.max.z - box.min.z, 1.20F);
  EXPECT_GT(SolidCount(BuildWindOccupancy(domain, mesh)), 20U);
}

}  // namespace
}  // namespace sph::sim
