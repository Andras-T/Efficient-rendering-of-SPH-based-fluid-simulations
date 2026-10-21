#include "CubeMesh.hpp"
#include "sim/Mesh.hpp"

#include <glm/common.hpp>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace sph::sim {
namespace {

TEST(MeshTest, AcceptsTypicalMeshExtensions) {
  EXPECT_TRUE(MeshPathLooksSupported("car.gltf"));
  EXPECT_TRUE(MeshPathLooksSupported("car.glb"));
  EXPECT_TRUE(MeshPathLooksSupported("car.OBJ"));
  EXPECT_TRUE(MeshPathLooksSupported("car.stl"));
  EXPECT_TRUE(MeshPathLooksSupported("car.fbx"));
  EXPECT_FALSE(MeshPathLooksSupported("car.blend"));
  EXPECT_FALSE(MeshPathLooksSupported("car"));
}

TEST(MeshTest, LoadsObjStlAndGltfThenFitsTheTunnel) {
  const auto directory = std::filesystem::temp_directory_path() / "sph-mesh-test";
  std::filesystem::create_directories(directory);
  const auto obj = LoadObstacleMesh(test::WriteCubeObj(directory));
  const auto stl = LoadObstacleMesh(test::WriteCubeStl(directory));
  const auto gltf = LoadObstacleMesh(test::WriteCubeGltf(directory));
  EXPECT_GE(obj.size(), 36U);
  EXPECT_GE(stl.size(), 36U);
  EXPECT_GE(gltf.size(), 3U);
  EXPECT_THROW(static_cast<void>(LoadObstacleMesh("missing.fbx")), std::runtime_error);
  EXPECT_THROW(static_cast<void>(LoadObstacleMesh("car.blend")), std::invalid_argument);

  const CarDomain domain;
  auto fitted = obj;
  FitMeshToTunnel(fitted, domain);
  glm::vec3 mn = fitted.front().position;
  glm::vec3 mx = mn;
  for (const MeshVertex& vertex : fitted) {
    mn = glm::min(mn, vertex.position);
    mx = glm::max(mx, vertex.position);
  }
  EXPECT_GT(mn.x, domain.origin.x);
  EXPECT_LT(mx.x, CarDomainMax(domain).x);
  EXPECT_NEAR(mn.y, domain.origin.y + domain.cellSize * 1.25F, domain.cellSize);
}

TEST(MeshTest, VoxelizesAClosedCube) {
  const auto directory = std::filesystem::temp_directory_path() / "sph-mesh-test";
  std::filesystem::create_directories(directory);
  auto mesh = LoadObstacleMesh(test::WriteCubeObj(directory));
  const CarDomain domain;
  FitMeshToTunnel(mesh, domain);
  const auto occupancy = BuildWindOccupancy(domain, mesh);
  std::uint32_t solids = 0;
  std::uint32_t inlets = 0;
  for (const std::uint32_t cell : occupancy) {
    solids += cell == static_cast<std::uint32_t>(WindCell::Solid) ? 1U : 0U;
    inlets += cell == static_cast<std::uint32_t>(WindCell::Inlet) ? 1U : 0U;
  }
  EXPECT_GT(solids, 40U);
  EXPECT_GT(inlets, 20U);
}

}  // namespace
}  // namespace sph::sim
