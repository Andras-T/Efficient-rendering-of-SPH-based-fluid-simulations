#include "sim/Car.hpp"
#include "sim/WindProbe.hpp"
#include "sim/WindUnits.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace sph::sim {
namespace {

std::uint32_t At(const CarDomain& domain, std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return x + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
}

struct Lattice {
  CarDomain domain{};
  std::vector<glm::vec4> macros;
  std::vector<std::uint32_t> occupancy;
  float inletUx = DefaultLatticeSpeed;

  Lattice() {
    const std::size_t count =
        static_cast<std::size_t>(domain.grid.x) * domain.grid.y * domain.grid.z;
    macros.assign(count, glm::vec4{1.0F, inletUx, 0.0F, 0.0F});
    occupancy.assign(count, static_cast<std::uint32_t>(WindCell::Fluid));
    for (std::uint32_t z = 0; z < domain.grid.z; ++z) {
      for (std::uint32_t y = 0; y < domain.grid.y; ++y) {
        occupancy[At(domain, 0, y, z)] = static_cast<std::uint32_t>(WindCell::Wall);
        occupancy[At(domain, 1, y, z)] = static_cast<std::uint32_t>(WindCell::Inlet);
        occupancy[At(domain, domain.grid.x - 1, y, z)] =
            static_cast<std::uint32_t>(WindCell::Outlet);
        occupancy[At(domain, domain.grid.x - 2, y, z)] =
            static_cast<std::uint32_t>(WindCell::Outlet);
      }
    }
    for (std::uint32_t z = 0; z < domain.grid.z; ++z) {
      for (std::uint32_t x = 0; x < domain.grid.x; ++x) {
        occupancy[At(domain, x, 0, z)] = static_cast<std::uint32_t>(WindCell::Wall);
        occupancy[At(domain, x, domain.grid.y - 1, z)] = static_cast<std::uint32_t>(WindCell::Wall);
      }
    }
    for (std::uint32_t y = 0; y < domain.grid.y; ++y) {
      for (std::uint32_t x = 0; x < domain.grid.x; ++x) {
        occupancy[At(domain, x, y, 0)] = static_cast<std::uint32_t>(WindCell::Wall);
        occupancy[At(domain, x, y, domain.grid.z - 1)] = static_cast<std::uint32_t>(WindCell::Wall);
      }
    }
  }

  void solidBlock(std::uint32_t x0, std::uint32_t x1, std::uint32_t y0, std::uint32_t y1,
                  std::uint32_t z0, std::uint32_t z1) {
    for (std::uint32_t z = z0; z < z1; ++z) {
      for (std::uint32_t y = y0; y < y1; ++y) {
        for (std::uint32_t x = x0; x < x1; ++x) {
          occupancy[At(domain, x, y, z)] = static_cast<std::uint32_t>(WindCell::Solid);
          macros[At(domain, x, y, z)] = {1.0F, 0.0F, 0.0F, 0.0F};
        }
      }
    }
  }

  [[nodiscard]] glm::vec3 center(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
    return domain.origin + domain.cellSize * (glm::vec3(x, y, z) + glm::vec3(0.5F));
  }
};

TEST(WindUnitsTest, DefaultInletIs180Kmh) {
  EXPECT_NEAR(LatticeToKmh(DefaultLatticeSpeed), 180.0F, 1.0e-3F);
  EXPECT_NEAR(LatticeToMs(DefaultLatticeSpeed), 50.0F, 1.0e-3F);
  EXPECT_NEAR(KmhToLattice(180.0F), DefaultLatticeSpeed, 1.0e-5F);
  EXPECT_NEAR(KmhToLattice(LatticeToKmh(0.10F)), 0.10F, 1.0e-5F);
  EXPECT_GE(KmhToLattice(WindKmhMin), LatticeSpeedMin);
  EXPECT_LE(KmhToLattice(WindKmhMax), LatticeSpeedMax);
  EXPECT_NEAR(ClampLatticeSpeed(0.0F), LatticeSpeedMin, 1.0e-6F);
  EXPECT_NEAR(ClampLatticeSpeed(1.0F), LatticeSpeedMax, 1.0e-6F);
}

TEST(WindProbeTest, InletPointReportsFreestreamSpeed) {
  const Lattice lattice;
  const glm::vec3 inlet = lattice.center(3, lattice.domain.grid.y / 2, lattice.domain.grid.z / 2);
  const WindProbe probe =
      SampleWindPoint(lattice.domain, lattice.macros, lattice.occupancy, lattice.inletUx, inlet);
  ASSERT_TRUE(probe.hit);
  EXPECT_FALSE(probe.onBody);
  EXPECT_FALSE(probe.onGround);
  EXPECT_NEAR(probe.speedLattice, lattice.inletUx, 1.0e-4F);
  EXPECT_NEAR(probe.speedMs, LatticeToMs(lattice.inletUx), 1.0e-3F);
  EXPECT_NEAR(probe.speedKmh, LatticeToKmh(lattice.inletUx), 1.0e-2F);
  EXPECT_NEAR(probe.pressure, 1.0F / 3.0F, 1.0e-4F);
  EXPECT_NEAR(probe.cp, 0.0F, 5.0e-3F);
  EXPECT_TRUE(std::isfinite(probe.cp));
}

TEST(WindProbeTest, RayHitsSolidAsOnBody) {
  Lattice lattice;
  lattice.solidBlock(40, 48, 8, 16, 12, 20);
  const glm::vec3 front = lattice.center(8, 12, 16);
  const WindProbe probe = ProbeWindRay(lattice.domain, lattice.macros, lattice.occupancy,
                                       lattice.inletUx, front, glm::vec3{1.0F, 0.0F, 0.0F});
  ASSERT_TRUE(probe.hit);
  EXPECT_TRUE(probe.onBody);
  EXPECT_FALSE(probe.onGround);
  EXPECT_TRUE(std::isfinite(probe.cp));
  EXPECT_TRUE(std::isfinite(probe.pressure));
  EXPECT_TRUE(std::isfinite(probe.speedMs));
  EXPECT_GT(probe.position.x, front.x);
}

TEST(WindProbeTest, RayThroughClearAirStaysOffBody) {
  const Lattice lattice;
  const glm::vec3 above = lattice.center(8, lattice.domain.grid.y - 3, lattice.domain.grid.z / 2);
  const WindProbe probe = ProbeWindRay(lattice.domain, lattice.macros, lattice.occupancy,
                                       lattice.inletUx, above, glm::vec3{1.0F, 0.0F, 0.0F});
  ASSERT_TRUE(probe.hit);
  EXPECT_FALSE(probe.onBody);
  EXPECT_NEAR(probe.speedKmh, LatticeToKmh(lattice.inletUx), 1.0F);
}

TEST(WindProbeTest, RayPassesEnclosureWallsThenHitsTheBody) {
  Lattice lattice;
  lattice.solidBlock(40, 48, 8, 16, 12, 20);
  const glm::vec3 above = lattice.center(44, lattice.domain.grid.y - 2, 16);
  const glm::vec3 origin = above + glm::vec3(0.0F, 0.20F, 0.0F);
  const WindProbe probe = ProbeWindRay(lattice.domain, lattice.macros, lattice.occupancy,
                                       lattice.inletUx, origin, glm::vec3{0.0F, -1.0F, 0.0F});
  ASSERT_TRUE(probe.hit);
  EXPECT_TRUE(probe.onBody);
  EXPECT_FALSE(probe.onGround);
}

TEST(WindProbeTest, RaisedDensityReadsPositiveCp) {
  Lattice lattice;
  const glm::ivec3 cell{20, 10, 16};
  lattice.macros[At(lattice.domain, static_cast<std::uint32_t>(cell.x),
                    static_cast<std::uint32_t>(cell.y), static_cast<std::uint32_t>(cell.z))] = {
      1.04F, 0.02F, 0.0F, 0.0F};
  const WindProbe probe = SampleWindPoint(lattice.domain, lattice.macros, lattice.occupancy,
                                          lattice.inletUx, lattice.center(20, 10, 16));
  ASSERT_TRUE(probe.hit);
  EXPECT_GT(probe.cp, 0.05F);
  EXPECT_GT(probe.pressure, 1.0F / 3.0F);
}

}  // namespace
}  // namespace sph::sim
