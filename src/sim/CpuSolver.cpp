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
    if (particles[i].flags != ParticleFlagFluid) {
      particles[i].density = params.restDensity;
      particles[i].pressure = 0.0F;
      continue;
    }
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
  const bool dfsph = params.solver == SolverMethod::Dfsph;
  for (std::uint32_t i = 0; i < particles.size(); ++i) {
    if (particles[i].flags != ParticleFlagFluid) {
      particles[i].force = glm::vec3{0.0F};
      continue;
    }
    glm::vec3 force{0.0F};
    glm::vec3 xsph{0.0F};
    const Particle& self = particles[i];
    ForNeighbors(i, particles, neighbors, [&](std::uint32_t j, const Particle& other) {
      if (j == i) {
        return;
      }
      const glm::vec3 delta = self.position - other.position;
      const float distance = glm::length(delta);
      const glm::vec3 grad = SpikyGradient(delta, h);
      const float otherDensity = std::max(other.density, KernelEpsilon);
      if (!dfsph) {
        float pressureTerm = self.pressure / (self.density * self.density);
        if (other.flags != ParticleFlagBoundary) {
          pressureTerm += other.pressure / (other.density * other.density);
        }
        force -= other.mass * pressureTerm * grad;
        xsph += other.mass * (other.velocity - self.velocity) / otherDensity *
                Poly6(glm::dot(delta, delta), h);
      }
      const float lap = ViscosityLaplacian(distance, h);
      force +=
          params.viscosity * other.mass * ((other.velocity - self.velocity) / otherDensity) * lap;
    });
    particles[i].force = force;
    if (!dfsph) {
      particles[i].velocity += params.xsph * xsph;
    }
  }
}

glm::vec3 CpuNonPressureAccel(const Particle& particle, const SimParams& params) {
  return particle.force + params.gravity;
}

void LimitSpeedCpu(Particle& particle, const SimParams& params) {
  const float vmax = params.cflSpeed();
  const float speed = glm::length(particle.velocity);
  if (speed > vmax) {
    particle.velocity *= vmax / speed;
  }
}

void CollideCpu(Particle& particle, const SimParams& params) {
  const float radius = params.collisionRadius();
  const glm::vec3 minBound = params.domainMin + glm::vec3(radius);
  const glm::vec3 maxBound = params.domainMax - glm::vec3(radius);
  if (!std::isfinite(particle.position.x) || !std::isfinite(particle.velocity.x)) {
    particle.position = 0.5F * (minBound + maxBound);
    particle.velocity = glm::vec3{0.0F};
    return;
  }
  for (int axis = 0; axis < 3; ++axis) {
    const float lo = minBound[axis];
    const float hi = maxBound[axis];
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
  LimitSpeedCpu(particle, params);
}

void Integrate(std::span<Particle> particles, const SimParams& params, float timeStep) {
  for (Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      particle.velocity = glm::vec3{0.0F};
      particle.force = glm::vec3{0.0F};
      continue;
    }
    particle.velocity += CpuNonPressureAccel(particle, params) * timeStep;
    particle.position += particle.velocity * timeStep;
    CollideCpu(particle, params);
  }
}

void ComputeAlpha(std::span<Particle> particles, const SimParams& params) {
  if (particles.empty()) {
    return;
  }
  NeighborGrid neighbors;
  BuildNeighbors(particles, neighbors, params);
  const float h = params.supportRadius();
  for (std::uint32_t i = 0; i < particles.size(); ++i) {
    if (particles[i].flags != ParticleFlagFluid) {
      particles[i].alpha = 0.0F;
      particles[i].kappa = 0.0F;
      continue;
    }
    glm::vec3 sumGrad{0.0F};
    float sumNorm2 = 0.0F;
    ForNeighbors(i, particles, neighbors, [&](std::uint32_t j, const Particle& other) {
      if (j == i) {
        return;
      }
      const glm::vec3 grad = other.mass * SpikyGradient(particles[i].position - other.position, h);
      sumGrad += grad;
      sumNorm2 += glm::dot(grad, grad);
    });
    particles[i].alpha = 1.0F / std::max(1.0e-6F, glm::dot(sumGrad, sumGrad) + sumNorm2);
  }
}

void ApplyKappa(std::span<Particle> particles, const SimParams& params, bool densityPass) {
  if (particles.empty()) {
    return;
  }
  NeighborGrid neighbors;
  BuildNeighbors(particles, neighbors, params);
  const float h = params.supportRadius();
  const float dt = std::max(params.timeStep, KernelEpsilon);
  for (std::uint32_t i = 0; i < particles.size(); ++i) {
    if (particles[i].flags != ParticleFlagFluid) {
      particles[i].kappa = 0.0F;
      continue;
    }
    float source = 0.0F;
    ForNeighbors(i, particles, neighbors, [&](std::uint32_t j, const Particle& other) {
      if (j == i) {
        return;
      }
      const glm::vec3 grad = SpikyGradient(particles[i].position - other.position, h);
      source += other.mass * glm::dot(particles[i].velocity - other.velocity, grad);
    });
    constexpr float KappaMax = 2.0e4F;
    if (densityPass) {
      const float predicted = particles[i].density + dt * source;
      particles[i].kappa = std::clamp(
          ((predicted - params.restDensity) / (dt * dt)) * particles[i].alpha, 0.0F, KappaMax);
    } else {
      particles[i].divergence = source / particles[i].density;
      particles[i].kappa = std::clamp((source / dt) * particles[i].alpha, -KappaMax, KappaMax);
    }
  }
  for (std::uint32_t i = 0; i < particles.size(); ++i) {
    if (particles[i].flags != ParticleFlagFluid) {
      continue;
    }
    glm::vec3 correction{0.0F};
    ForNeighbors(i, particles, neighbors, [&](std::uint32_t j, const Particle& other) {
      if (j == i) {
        return;
      }
      const glm::vec3 grad = SpikyGradient(particles[i].position - other.position, h);
      float kappaSum = particles[i].kappa;
      if (other.flags != ParticleFlagBoundary) {
        kappaSum += other.kappa;
      }
      correction -= other.mass * kappaSum * grad;
    });
    particles[i].velocity += correction * dt;
    LimitSpeedCpu(particles[i], params);
  }
}

void StepDfsph(std::span<Particle> particles, const SimParams& params) {
  ComputeDensityAndPressure(particles, params);
  ComputeForces(particles, params);
  const float dt = params.timeStep;
  for (Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      particle.velocity = glm::vec3{0.0F};
      particle.force = glm::vec3{0.0F};
      continue;
    }
    particle.velocity += CpuNonPressureAccel(particle, params) * dt;
    LimitSpeedCpu(particle, params);
  }
  ComputeAlpha(particles, params);
  const std::uint32_t densityIters = std::max(1U, params.densityIterations);
  for (std::uint32_t iter = 0; iter < densityIters; ++iter) {
    ComputeDensityAndPressure(particles, params);
    ApplyKappa(particles, params, true);
  }
  ApplyKappa(particles, params, false);
  for (Particle& particle : particles) {
    if (particle.flags != ParticleFlagFluid) {
      continue;
    }
    particle.position += particle.velocity * dt;
    CollideCpu(particle, params);
  }
}

void StepCpu(std::span<Particle> particles, const SimParams& params, std::uint32_t substeps) {
  for (std::uint32_t step = 0; step < substeps; ++step) {
    if (params.solver == SolverMethod::Dfsph) {
      StepDfsph(particles, params);
    } else {
      ComputeDensityAndPressure(particles, params);
      ComputeForces(particles, params);
      Integrate(particles, params, params.timeStep);
    }
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
  const float margin = params.supportRadius();
  float packed = 0.0F;
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
    if (interior) {
      packed = std::max(packed, particle.density);
    }
  }
  if (packed > KernelEpsilon) {
    params.restDensity = packed;
  }
}

}  // namespace sph::sim
