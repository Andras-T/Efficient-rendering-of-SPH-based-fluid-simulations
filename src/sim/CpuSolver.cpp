#include "sim/CpuSolver.hpp"

#include "sim/Grid.hpp"
#include "sim/Kernels.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sph::sim {

namespace {

struct NeighborGrid {
  Grid grid;
  std::vector<std::uint32_t> cellOf;
  std::vector<std::uint32_t> counts;
  std::vector<std::uint32_t> offsets;
  std::vector<std::uint32_t> fill;
  std::vector<std::uint32_t> dest;
  std::vector<Particle> sorted;
};

void BuildNeighbors(std::span<Particle> particles, NeighborGrid& neighbors,
                    const SimParams& params) {
  neighbors.grid = MakeGrid(params);
  const auto n = static_cast<std::uint32_t>(particles.size());
  const std::uint32_t cells = neighbors.grid.cellCount;
  neighbors.cellOf.resize(n);
  neighbors.counts.assign(cells, 0);
  neighbors.offsets.resize(cells);
  neighbors.fill.resize(cells);
  neighbors.dest.resize(n);
  neighbors.sorted.resize(n);

  for (std::uint32_t i = 0; i < n; ++i) {
    const std::uint32_t cell = CellIndex(particles[i].position, neighbors.grid);
    neighbors.cellOf[i] = cell;
    ++neighbors.counts[cell];
  }
  ExclusiveScan(neighbors.counts, neighbors.offsets);
  CountingSort(neighbors.cellOf, neighbors.offsets, neighbors.fill, neighbors.dest);
  for (std::uint32_t i = 0; i < n; ++i) {
    neighbors.sorted[neighbors.dest[i]] = particles[i];
  }
  for (std::uint32_t i = 0; i < n; ++i) {
    particles[i] = neighbors.sorted[i];
  }
}

template <typename Fn>
void ForNeighbors(std::uint32_t index, std::span<const Particle> particles,
                  const NeighborGrid& neighbors, const Fn& fn) {
  const glm::ivec3 center = CellCoords(particles[index].position, neighbors.grid);
  for (int z = -1; z <= 1; ++z) {
    for (int y = -1; y <= 1; ++y) {
      for (int x = -1; x <= 1; ++x) {
        const glm::ivec3 cell = center + glm::ivec3{x, y, z};
        if (cell.x < 0 || cell.y < 0 || cell.z < 0 ||
            cell.x >= static_cast<int>(neighbors.grid.size.x) ||
            cell.y >= static_cast<int>(neighbors.grid.size.y) ||
            cell.z >= static_cast<int>(neighbors.grid.size.z)) {
          continue;
        }
        const std::uint32_t linear = CellIndex(cell, neighbors.grid);
        const std::uint32_t begin = neighbors.offsets[linear];
        const std::uint32_t end = begin + neighbors.counts[linear];
        for (std::uint32_t j = begin; j < end; ++j) {
          fn(j, particles[j]);
        }
      }
    }
  }
}

}  // namespace

void ComputeDensityAndPressure(std::span<Particle> particles, const SimParams& params) {
  if (particles.empty()) {
    return;
  }
  NeighborGrid neighbors;
  BuildNeighbors(particles, neighbors, params);
  const float h = params.supportRadius();
  for (std::uint32_t i = 0; i < particles.size(); ++i) {
    float density = 0.0F;
    ForNeighbors(i, particles, neighbors, [&](std::uint32_t /*j*/, const Particle& other) {
      const glm::vec3 delta = particles[i].position - other.position;
      density += other.mass * Poly6(glm::dot(delta, delta), h);
    });
    particles[i].density = std::max(density, KernelEpsilon);
    particles[i].pressure =
        TaitPressure(particles[i].density, params.restDensity, params.stiffness(), params.eosGamma);
  }
}

void ComputeForces(std::span<Particle> particles, const SimParams& params) {
  if (particles.empty()) {
    return;
  }
  NeighborGrid neighbors;
  BuildNeighbors(particles, neighbors, params);
  const float h = params.supportRadius();
  for (std::uint32_t i = 0; i < particles.size(); ++i) {
    glm::vec3 force{0.0F};
    const Particle& self = particles[i];
    ForNeighbors(i, particles, neighbors, [&](std::uint32_t j, const Particle& other) {
      if (j == i) {
        return;
      }
      const glm::vec3 delta = self.position - other.position;
      const float distance = glm::length(delta);
      const glm::vec3 grad = SpikyGradient(delta, h);
      const float pressureTerm = (self.pressure / (self.density * self.density)) +
                                 (other.pressure / (other.density * other.density));
      force -= other.mass * pressureTerm * grad;
      const float lap = ViscosityLaplacian(distance, h);
      force +=
          params.viscosity * other.mass * ((other.velocity - self.velocity) / other.density) * lap;
    });
    particles[i].force = force;
  }
}

void Integrate(std::span<Particle> particles, const SimParams& params, float timeStep) {
  const glm::vec3 min = params.domainMin;
  const glm::vec3 max = params.domainMax;
  const float radius = params.particleSpacing * 0.5F;
  for (Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      particle.velocity = glm::vec3{0.0F};
      particle.force = glm::vec3{0.0F};
      continue;
    }
    const glm::vec3 acceleration = particle.force / particle.density + params.gravity;
    particle.velocity += acceleration * timeStep;
    particle.position += particle.velocity * timeStep;
    for (int axis = 0; axis < 3; ++axis) {
      const float lo = min[axis] + radius;
      const float hi = max[axis] - radius;
      if (particle.position[axis] < lo) {
        particle.position[axis] = lo;
        if (particle.velocity[axis] < 0.0F) {
          particle.velocity[axis] *= -params.restitution;
        }
      } else if (particle.position[axis] > hi) {
        particle.position[axis] = hi;
        if (particle.velocity[axis] > 0.0F) {
          particle.velocity[axis] *= -params.restitution;
        }
      }
    }
  }
}

void StepCpu(std::span<Particle> particles, const SimParams& params, std::uint32_t substeps) {
  for (std::uint32_t step = 0; step < substeps; ++step) {
    ComputeDensityAndPressure(particles, params);
    ComputeForces(particles, params);
    Integrate(particles, params, params.timeStep);
  }
}

float MeanFluidDensity(std::span<const Particle> particles) {
  double sum = 0.0;
  std::uint32_t count = 0;
  for (const Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    sum += static_cast<double>(particle.density);
    ++count;
  }
  return count == 0 ? 0.0F : static_cast<float>(sum / static_cast<double>(count));
}

float MeanInteriorDensity(std::span<const Particle> particles, const SimParams& params) {
  const float margin = params.supportRadius();
  double sum = 0.0;
  std::uint32_t count = 0;
  for (const Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    bool interior = true;
    for (int axis = 0; axis < 3; ++axis) {
      if (particle.position[axis] < params.domainMin[axis] + margin ||
          particle.position[axis] > params.domainMax[axis] - margin) {
        interior = false;
      }
    }
    if (!interior) {
      continue;
    }
    sum += static_cast<double>(particle.density);
    ++count;
  }
  return count == 0 ? 0.0F : static_cast<float>(sum / static_cast<double>(count));
}

void CalibrateRestDensity(std::span<Particle> particles, SimParams& params) {
  ComputeDensityAndPressure(particles, params);
  float packed = 0.0F;
  for (const Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    packed = std::max(packed, particle.density);
  }
  if (packed > KernelEpsilon) {
    params.restDensity = packed;
  }
}

}  // namespace sph::sim
