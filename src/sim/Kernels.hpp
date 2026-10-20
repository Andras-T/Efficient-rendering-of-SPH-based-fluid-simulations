#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace sph::sim {

constexpr float Pi = std::numbers::pi_v<float>;
constexpr float KernelEpsilon = 1.0e-8F;

inline float Poly6(float distanceSquared, float kernelRadius) {
  const float h2 = kernelRadius * kernelRadius;
  if (distanceSquared >= h2) {
    return 0.0F;
  }
  const float scale = 315.0F / (64.0F * Pi * std::pow(kernelRadius, 9.0F));
  const float delta = h2 - distanceSquared;
  return scale * delta * delta * delta;
}

inline glm::vec3 SpikyGradient(glm::vec3 offset, float kernelRadius) {
  const float distance = glm::length(offset);
  if (distance <= KernelEpsilon || distance >= kernelRadius) {
    return glm::vec3{0.0F};
  }
  const float scale = -45.0F / (Pi * std::pow(kernelRadius, 6.0F));
  const float gap = kernelRadius - distance;
  return offset * ((scale * gap * gap) / distance);
}

inline float ViscosityLaplacian(float distance, float kernelRadius) {
  if (distance >= kernelRadius) {
    return 0.0F;
  }
  return (45.0F / (Pi * std::pow(kernelRadius, 6.0F))) * (kernelRadius - distance);
}

inline float TaitPressure(float density, float restDensity, float stiffness, float gamma) {
  const float ratio = std::max(density, KernelEpsilon) / restDensity;
  return std::max(0.0F, stiffness * (std::pow(ratio, gamma) - 1.0F));
}

constexpr float DfsphKappaMax = 12.0F;
constexpr float DfsphOmega = 0.35F;

inline float DfsphAlphaFromDenom(float denom, float dt) {
  const float alpha = 1.0F / std::max(1.0e-6F, denom);
  const float alphaMax = 0.02F * dt * dt;
  return std::min(alpha, alphaMax);
}

inline float DfsphDensityKappa(float predicted, float restDensity, float dt, float alpha) {
  return std::clamp(((predicted - restDensity) / (dt * dt)) * alpha, 0.0F, DfsphKappaMax);
}

inline float DfsphDivergenceKappa(float source, float density, float restDensity, float dt,
                                  float alpha) {
  if (density < 0.85F * restDensity) {
    return 0.0F;
  }
  return std::clamp((source / dt) * alpha, -DfsphKappaMax, DfsphKappaMax);
}

}  // namespace sph::sim
