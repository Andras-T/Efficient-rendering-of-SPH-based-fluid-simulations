#include "sim/AeroBodies.hpp"
#include "sim/Mesh.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace sph::sim {
namespace {

TEST(AeroBenchmark, PublishedDragCoefficientsMatchWellKnownValues) {
  // Hoerner 1965 ch. 3: subcritical sphere and face-on cube.
  EXPECT_FLOAT_EQ(SphereCdSubcritical, 0.47F);
  EXPECT_FLOAT_EQ(CubeCdFaceOn, 1.05F);
  // Ahmed, Ramm & Faltin, SAE 840300: 25° slant.
  EXPECT_FLOAT_EQ(AhmedCd25Deg, 0.230F);
  EXPECT_GT(CubeCdFaceOn, SphereCdSubcritical);
  EXPECT_GT(SphereCdSubcritical, AhmedCd25Deg);
}

TEST(AeroBenchmark, SchillerNaumannMatchesPublishedSphereCheckpoints) {
  // Schiller & Naumann 1933; checkpoints agree with Clift, Grace & Weber
  // (1978) Bubbles, Drops, and Particles, Table 5.1 around Re = 10 and 100.
  EXPECT_NEAR(SphereCdSchillerNaumann(1.0F), 27.6F, 0.05F);
  EXPECT_NEAR(SphereCdSchillerNaumann(10.0F), 4.17F, 0.08F);
  EXPECT_NEAR(SphereCdSchillerNaumann(100.0F), 1.09F, 0.05F);
}

TEST(AeroBenchmark, LatticeReynoldsIsLowForTheDemoGrid) {
  EXPECT_NEAR(LbmLatticeViscosity, (0.60F - 0.5F) / 3.0F, 1.0e-6F);
  // ~12-cell body at U = 0.08, tau = 0.60 → Re_D ≈ 29, not a high-Re tunnel.
  EXPECT_NEAR(LatticeReynolds(12.0F), 28.8F, 0.2F);
  EXPECT_LT(LatticeReynolds(12.0F), 80.0F);
}

TEST(AeroBenchmark, ClassicBodiesFitTheLatticeAndKeepABluffSilhouette) {
  const CarDomain domain;
  auto measure = [&](std::vector<MeshVertex> mesh) {
    PlaceBluffBody(mesh, domain);
    return MeasureSolidOccupancy(domain, BuildWindOccupancy(domain, mesh));
  };

  const BodyOccupancy cube = measure(BuildBenchmarkCube());
  const BodyOccupancy sphere = measure(BuildBenchmarkSphere());
  const BodyOccupancy ahmed = measure(BuildBenchmarkAhmed());

  EXPECT_GT(cube.solids, 40U);
  EXPECT_GT(sphere.solids, 40U);
  EXPECT_GT(ahmed.solids, 40U);
  EXPECT_GT(cube.frontalCells, 20U);
  EXPECT_GT(sphere.frontalCells, 20U);
  EXPECT_GT(ahmed.frontalCells, 20U);

  EXPECT_LE(cube.extentX, domain.grid.x - 8);
  EXPECT_LE(cube.extentY, domain.grid.y - 4);
  EXPECT_LE(cube.extentZ, domain.grid.z - 4);
  EXPECT_GE(cube.extentY, 6U);
  EXPECT_LE(cube.extentY, 14U);
  EXPECT_GE(sphere.extentY, 6U);
  EXPECT_LE(sphere.extentY, 14U);
  EXPECT_GE(ahmed.extentX, 10U);
  EXPECT_LE(ahmed.extentX, 28U);

  EXPECT_GT(ahmed.extentX, ahmed.extentY);
  EXPECT_NEAR(static_cast<float>(cube.extentX), static_cast<float>(cube.extentY), 3.0F);
  EXPECT_NEAR(static_cast<float>(sphere.extentY), static_cast<float>(sphere.extentZ), 3.0F);
}

}  // namespace
}  // namespace sph::sim
