#pragma once

#include "sim/Particle.hpp"
#include "sim/SimParams.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace sph::sim {

void ComputeDensityAndPressure(std::span<Particle> particles, const SimParams& params);

void ComputeForces(std::span<Particle> particles, const SimParams& params);

void Integrate(std::span<Particle> particles, const SimParams& params, float timeStep);

void StepCpu(std::span<Particle> particles, const SimParams& params, std::uint32_t substeps);

void CalibrateRestDensity(std::span<Particle> particles, SimParams& params);

[[nodiscard]] float MeanFluidDensity(std::span<const Particle> particles);

[[nodiscard]] float MeanInteriorDensity(std::span<const Particle> particles,
                                        const SimParams& params);

}  // namespace sph::sim
