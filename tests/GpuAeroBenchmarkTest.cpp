#include "gpu/Context.hpp"
#include "sim/AeroBodies.hpp"
#include "sim/Mesh.hpp"
#include "sim/WindSolver.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <utility>
#include <vector>

namespace sph {
namespace {

void SettleAero(sim::WindSolver& wind) {
  for (int i = 0; i < 20; ++i) {
    wind.refreshAero();
  }
}

sim::WindAero RunBluffBody(sim::WindSolver& wind, const std::filesystem::path& path) {
  auto mesh = sim::LoadObstacleMesh(path);
  sim::PlaceBluffBody(mesh, wind.domain());
  wind.setObstacle(std::move(mesh));
  wind.step(96);
  SettleAero(wind);
  return wind.aero();
}

void ExpectFiniteAero(const sim::WindAero& aero) {
  EXPECT_GT(aero.drag, 0.0F);
  EXPECT_GT(aero.cd, 0.0F);
  EXPECT_GT(aero.frontalArea, 0.0F);
  EXPECT_TRUE(std::isfinite(aero.cd));
  EXPECT_TRUE(std::isfinite(aero.cl));
  EXPECT_TRUE(std::isfinite(aero.lift));
}

TEST(GpuAeroBenchmark, ClassicBluffBodiesStayInPublishedCdBands) {
  // Published high-Re values (Hoerner 1965; Ahmed et al. SAE 840300) plus the
  // low-Re sphere correlation (Schiller & Naumann 1933). This lattice is
  // Re_D ~ 20–30, so the bands are coarse — not a tunnel certificate.
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context);
  ASSERT_NEAR(wind.relaxationTime(), sim::LbmRelaxationTime, 1.0e-6F);

  const auto directory = std::filesystem::temp_directory_path() / "sph-aero-bench";
  std::filesystem::create_directories(directory);
  const auto cubePath = directory / "cube.obj";
  const auto spherePath = directory / "sphere.obj";
  const auto ahmedPath = directory / "ahmed.obj";
  sim::WriteMeshObj(cubePath, sim::BuildBenchmarkCube());
  sim::WriteMeshObj(spherePath, sim::BuildBenchmarkSphere());
  sim::WriteMeshObj(ahmedPath, sim::BuildBenchmarkAhmed());

  const sim::WindAero cube = RunBluffBody(wind, cubePath);
  ExpectFiniteAero(cube);
  EXPECT_GE(cube.cd, sim::CubeCdBandMin) << "cube Cd vs Hoerner face-on " << sim::CubeCdFaceOn;
  EXPECT_LE(cube.cd, sim::CubeCdBandMax) << "cube Cd vs Hoerner face-on " << sim::CubeCdFaceOn;
  EXPECT_LT(std::abs(cube.cl), sim::BluffClAbsMax);

  const sim::WindAero sphere = RunBluffBody(wind, spherePath);
  ExpectFiniteAero(sphere);
  const float sphereRe = sim::LatticeReynolds(9.0F, wind.inletSpeed());
  const float sphereLowRe = sim::SphereCdSchillerNaumann(sphereRe);
  EXPECT_GE(sphere.cd, sim::SphereCdBandMin)
      << "sphere Cd vs Hoerner " << sim::SphereCdSubcritical << " and Schiller–Naumann "
      << sphereLowRe << " at Re_D≈" << sphereRe;
  EXPECT_LE(sphere.cd, sim::SphereCdBandMax) << "sphere Cd vs Hoerner " << sim::SphereCdSubcritical
                                             << " and Schiller–Naumann " << sphereLowRe;
  EXPECT_LT(std::abs(sphere.cl), sim::BluffClAbsMax);

  const sim::WindAero ahmed = RunBluffBody(wind, ahmedPath);
  ExpectFiniteAero(ahmed);
  EXPECT_GE(ahmed.cd, sim::AhmedCdBandMin) << "Ahmed Cd vs SAE 840300 25° " << sim::AhmedCd25Deg;
  EXPECT_LE(ahmed.cd, sim::AhmedCdBandMax) << "Ahmed Cd vs SAE 840300 25° " << sim::AhmedCd25Deg;
  EXPECT_LT(std::abs(ahmed.cl), sim::AhmedClAbsMax);

  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
