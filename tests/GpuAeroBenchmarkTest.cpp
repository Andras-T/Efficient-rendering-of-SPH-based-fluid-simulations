#include "gpu/Context.hpp"
#include "sim/AeroBodies.hpp"
#include "sim/Mesh.hpp"
#include "sim/WindSolver.hpp"

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace sph {
namespace {

void SettleAero(sim::WindSolver& wind) {
  for (int i = 0; i < 24; ++i) {
    wind.refreshAero();
  }
}

struct BodyRun {
  sim::WindAero aero;
  sim::BodyOccupancy occupancy;
  float reD = 0.0F;
  float reL = 0.0F;
  float sn = 0.0F;
};

std::string ReLabel(const BodyRun& run) {
  return "Re_D=" + std::to_string(run.reD) + " Re_L=" + std::to_string(run.reL);
}

std::string AeroLabel(const char* name, const BodyRun& run) {
  return std::string(name) + " Cd=" + std::to_string(run.aero.cd) +
         " Cl=" + std::to_string(run.aero.cl) + " drag=" + std::to_string(run.aero.drag) +
         " A=" + std::to_string(run.aero.frontalArea) + " " + ReLabel(run);
}

BodyRun FinishRun(sim::WindSolver& wind, std::uint32_t steps) {
  wind.step(steps);
  SettleAero(wind);
  BodyRun run;
  run.aero = wind.aero();
  std::vector<std::uint32_t> occupancy(wind.cellCount());
  wind.downloadOccupancy(occupancy);
  run.occupancy = sim::MeasureSolidOccupancy(wind.domain(), occupancy);
  run.reD = sim::LatticeReynolds(sim::DiameterCells(run.occupancy), wind.inletSpeed(),
                                 wind.relaxationTime());
  run.reL = sim::LatticeReynolds(static_cast<float>(run.occupancy.extentX), wind.inletSpeed(),
                                 wind.relaxationTime());
  run.sn = sim::SphereCdSchillerNaumann(run.reD);
  return run;
}

BodyRun MeasureMeshBody(sim::WindSolver& wind, std::vector<sim::MeshVertex> mesh,
                        const sim::BluffPlacement& placement, std::uint32_t steps) {
  sim::PlaceBluffBody(mesh, wind.domain(), placement);
  wind.setObstacle(std::move(mesh));
  return FinishRun(wind, steps);
}

BodyRun MeasurePainted(sim::WindSolver& wind, std::span<const std::uint32_t> occupancy,
                       std::uint32_t steps) {
  wind.setOccupancy(occupancy);
  return FinishRun(wind, steps);
}

void ExpectFiniteAero(const sim::WindAero& aero) {
  EXPECT_GT(aero.drag, 0.0F);
  EXPECT_GT(aero.cd, 0.0F);
  EXPECT_GT(aero.frontalArea, 0.0F);
  EXPECT_TRUE(std::isfinite(aero.cd));
  EXPECT_TRUE(std::isfinite(aero.cl));
  EXPECT_TRUE(std::isfinite(aero.lift));
}

TEST(GpuAeroBenchmark, ClassicBluffBodiesRankAndMatchSchillerNaumann) {
  // Live D3Q19 solver on the demo 96×24×32 lattice. Cube and sphere are painted
  // in free stream; Ahmed sits at SAE-like ride height. Ranking is Hoerner /
  // Ahmed order. Sphere Cd is judged against Schiller–Naumann at lattice Re_D,
  // not Hoerner 0.47. Not a tunnel certificate.
  const gpu::Context context({.validation = true});
  // Same 96×24×32 box as the interactive tunnel; tau/U are test-only so Re_D
  // can rank shapes. The HUD tunnel stays tau=0.60, U=0.08.
  sim::WindSolver wind(
      context, {.tau = 0.58F, .inletUx = 0.09F, .loadDefaultCar = false, .advectOverlays = false});
  ASSERT_NEAR(wind.relaxationTime(), 0.58F, 1.0e-6F);

  const glm::vec3 mid =
      wind.domain().origin + wind.domain().cellSize * glm::vec3(32.5F, 12.5F, 16.5F);
  auto cubeOcc = sim::BuildWindOccupancy(wind.domain(), {});
  sim::PaintSolidCube(wind.domain(), cubeOcc, mid, 8.0F);
  const BodyRun cube = MeasurePainted(wind, cubeOcc, 200);
  ExpectFiniteAero(cube.aero);
  EXPECT_LT(std::abs(cube.aero.cl), sim::BluffClAbsMax) << AeroLabel("cube", cube);
  EXPECT_GE(cube.occupancy.minY, 3U) << AeroLabel("cube", cube);

  auto sphereOcc = sim::BuildWindOccupancy(wind.domain(), {});
  sim::PaintSolidSphere(wind.domain(), sphereOcc, mid, 4.0F);
  const BodyRun sphere = MeasurePainted(wind, sphereOcc, 200);
  ExpectFiniteAero(sphere.aero);
  EXPECT_LT(std::abs(sphere.aero.cl), sim::BluffClAbsMax) << AeroLabel("sphere", sphere);
  EXPECT_GE(sphere.occupancy.minY, 3U) << AeroLabel("sphere", sphere);
  EXPECT_GE(sphere.aero.cd, sim::SphereSnFactorMin * sphere.sn)
      << AeroLabel("sphere", sphere) << " SN=" << sphere.sn << " (Schiller & Naumann 1933)";
  EXPECT_LE(sphere.aero.cd, sim::SphereSnFactorMax * sphere.sn)
      << AeroLabel("sphere", sphere) << " SN=" << sphere.sn << " (Schiller & Naumann 1933)";

  const BodyRun ahmed = MeasureMeshBody(
      wind, sim::BuildBenchmarkAhmed(),
      {.sit = sim::BluffSit::Ground, .heightCells = 8.0F, .lengthCells = 40.0F}, 200);
  ExpectFiniteAero(ahmed.aero);
  EXPECT_LT(std::abs(ahmed.aero.cl), sim::AhmedClAbsMax) << AeroLabel("ahmed", ahmed);
  EXPECT_LE(ahmed.occupancy.minY, 5U) << AeroLabel("ahmed", ahmed);
  EXPECT_GE(ahmed.aero.cd, 0.5F * sim::AhmedCd25Deg)
      << AeroLabel("ahmed", ahmed) << " vs SAE 840300 Cd=" << sim::AhmedCd25Deg;

  EXPECT_GT(cube.aero.cd, sphere.aero.cd)
      << AeroLabel("cube", cube) << " vs " << AeroLabel("sphere", sphere)
      << " (Hoerner 1965 ch. 3: cube 1.05 > sphere 0.47)";
  EXPECT_GT(sphere.aero.cd, ahmed.aero.cd)
      << AeroLabel("sphere", sphere) << " vs " << AeroLabel("ahmed", ahmed)
      << " (SAE 840300 25° Cd=0.230)";

  if (cube.reD >= 30.0F) {
    EXPECT_LE(cube.aero.cd, sim::CubeHoernerFactorMax * sim::CubeCdFaceOn)
        << AeroLabel("cube", cube) << " vs 3× Hoerner " << sim::CubeCdFaceOn;
  }

  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuAeroBenchmark, SphereStokesCdMatchesLowReTheory) {
  // Dedicated slower lattice. Interactive tunnel stays 96×24×32 / tau=0.60.
  // Stokes Cd = 24/Re; Schiller–Naumann 1933 recovers that as Re → 0.
  constexpr float Tau = 1.00F;
  constexpr float Inlet = 0.02F;
  constexpr float RadiusCells = 2.5F;
  const sim::CarDomain domain{
      .origin = {-1.28F, 0.0F, -0.64F}, .cellSize = 0.04F, .grid = {64, 32, 32}};
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context, {.domain = domain,
                                 .tau = Tau,
                                 .inletUx = Inlet,
                                 .loadDefaultCar = false,
                                 .advectOverlays = false});
  auto occupancy = sim::BuildWindOccupancy(domain, {});
  const glm::vec3 center = domain.origin + domain.cellSize * glm::vec3(24.5F, 16.5F, 16.5F);
  sim::PaintSolidSphere(domain, occupancy, center, RadiusCells);
  const BodyRun run = MeasurePainted(wind, occupancy, 320);
  ExpectFiniteAero(run.aero);
  const float stokes = sim::SphereCdStokes(run.reD);
  EXPECT_LT(run.reD, 2.0F) << AeroLabel("stokes-sphere", run);
  EXPECT_GE(run.aero.cd, 0.50F * stokes)
      << AeroLabel("stokes-sphere", run) << " Stokes 24/Re=" << stokes << " SN=" << run.sn;
  EXPECT_LE(run.aero.cd, 2.20F * stokes)
      << AeroLabel("stokes-sphere", run) << " Stokes 24/Re=" << stokes << " SN=" << run.sn;
  EXPECT_GE(run.occupancy.minY, 10U) << AeroLabel("stokes-sphere", run);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(GpuAeroBenchmark, FinerSphereCdDropsTowardSchillerNaumann) {
  // D ≈ 16 on a test-only 64×40×40 lattice, tau=0.58. Cd should fall from the
  // coarse-grid value toward Schiller–Naumann. Hoerner 0.47 still needs Re~10^3.
  constexpr float Tau = 0.58F;
  constexpr float Inlet = 0.08F;
  constexpr float RadiusCells = 8.0F;
  const sim::CarDomain domain{
      .origin = {-1.28F, 0.0F, -0.80F}, .cellSize = 0.04F, .grid = {64, 40, 40}};
  const gpu::Context context({.validation = true});
  sim::WindSolver wind(context, {.domain = domain,
                                 .tau = Tau,
                                 .inletUx = Inlet,
                                 .loadDefaultCar = false,
                                 .advectOverlays = false});
  auto occupancy = sim::BuildWindOccupancy(domain, {});
  const glm::vec3 center = domain.origin + domain.cellSize * glm::vec3(24.5F, 20.5F, 20.5F);
  sim::PaintSolidSphere(domain, occupancy, center, RadiusCells);
  const BodyRun run = MeasurePainted(wind, occupancy, 256);
  ExpectFiniteAero(run.aero);
  EXPECT_GE(sim::DiameterCells(run.occupancy), 14.0F) << AeroLabel("fine-sphere", run);
  EXPECT_GT(run.reD, 40.0F) << AeroLabel("fine-sphere", run);
  EXPECT_GE(run.aero.cd, 0.45F * run.sn)
      << AeroLabel("fine-sphere", run) << " SN=" << run.sn << " (Schiller & Naumann 1933)";
  EXPECT_LE(run.aero.cd, 1.85F * run.sn)
      << AeroLabel("fine-sphere", run) << " SN=" << run.sn << " (Schiller & Naumann 1933)";
  EXPECT_LT(run.aero.cd, 4.0F) << AeroLabel("fine-sphere", run) << " SN=" << run.sn << " Hoerner "
                               << sim::SphereCdSubcritical;
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
