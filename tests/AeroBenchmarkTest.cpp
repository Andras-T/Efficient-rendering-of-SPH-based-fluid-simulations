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

TEST(AeroBenchmark, StokesCdIsTwentyFourOverRe) {
  EXPECT_NEAR(SphereCdStokes(1.0F), 24.0F, 1.0e-5F);
  EXPECT_NEAR(SphereCdStokes(0.1F), 240.0F, 1.0e-3F);
  // Schiller–Naumann recovers Stokes as Re → 0.
  EXPECT_NEAR(SphereCdSchillerNaumann(1.0e-3F) / SphereCdStokes(1.0e-3F), 1.0F, 0.02F);
  EXPECT_GT(SphereCdSchillerNaumann(1.0F), SphereCdStokes(1.0F));
}

TEST(AeroBenchmark, LatticeReynoldsIsLowForTheDemoGrid) {
  EXPECT_NEAR(LatticeViscosity(), (0.60F - 0.5F) / 3.0F, 1.0e-6F);
  // ~12-cell body at U = 0.08, tau = 0.60 → Re_D ≈ 29, not a high-Re tunnel.
  EXPECT_NEAR(LatticeReynolds(12.0F), 28.8F, 0.2F);
  EXPECT_LT(LatticeReynolds(12.0F), 80.0F);
}

TEST(AeroBenchmark, ClassicBodiesFitTheLatticeAndKeepABluffSilhouette) {
  const CarDomain domain;
  auto measure = [&](std::vector<MeshVertex> mesh, const BluffPlacement& placement) {
    PlaceBluffBody(mesh, domain, placement);
    return MeasureSolidOccupancy(domain, BuildWindOccupancy(domain, mesh));
  };

  const BodyOccupancy cube = measure(BuildBenchmarkCube(), {.sit = BluffSit::FreeStream});
  const BodyOccupancy sphere = measure(BuildBenchmarkSphere(), {.sit = BluffSit::FreeStream});
  const BodyOccupancy ahmed = measure(BuildBenchmarkAhmed(), {.sit = BluffSit::Ground});

  EXPECT_GT(cube.solids, 40U);
  EXPECT_GT(sphere.solids, 40U);
  EXPECT_GT(ahmed.solids, 40U);
  EXPECT_GT(cube.frontalCells, 20U);
  EXPECT_GT(sphere.frontalCells, 20U);
  EXPECT_GT(ahmed.frontalCells, 20U);

  EXPECT_GE(cube.minY, 3U) << "cube sits in free stream, not on the floor";
  EXPECT_GE(sphere.minY, 3U) << "sphere sits in free stream, not on the floor";
  EXPECT_GE(ahmed.minY, 1U);
  EXPECT_LE(ahmed.minY, 5U) << "Ahmed ride height is SAE-like, not free stream";

  EXPECT_LE(cube.extentY, 14U);
  EXPECT_LE(sphere.extentY, 14U);
  EXPECT_GE(ahmed.extentX, 10U);
  EXPECT_LE(ahmed.extentX, 32U);
  EXPECT_GT(ahmed.extentX, ahmed.extentY);
  EXPECT_NEAR(static_cast<float>(cube.extentX), static_cast<float>(cube.extentY), 3.0F);
  EXPECT_NEAR(static_cast<float>(sphere.extentY), static_cast<float>(sphere.extentZ), 3.0F);
}

TEST(AeroBenchmark, PaintedSphereFillsASolidBall) {
  const CarDomain domain;
  auto occupancy = BuildWindOccupancy(domain, {});
  const glm::vec3 center = domain.origin + domain.cellSize * glm::vec3(40.5F, 12.5F, 16.5F);
  PaintSolidSphere(domain, occupancy, center, 4.0F);
  const BodyOccupancy body = MeasureSolidOccupancy(domain, occupancy);
  EXPECT_GT(body.solids, 200U);
  EXPECT_GE(body.minY, 6U);
  EXPECT_NEAR(static_cast<float>(body.extentY), 8.0F, 2.0F);
  EXPECT_NEAR(static_cast<float>(body.extentZ), 8.0F, 2.0F);
}

}  // namespace
}  // namespace sph::sim
