#include "sim/Scene.hpp"

#include "sim/CpuSolver.hpp"
#include "sim/Kernels.hpp"
#include "sim/Tank.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <stdexcept>

namespace sph::sim {

namespace {

void AddParticle(Scene& scene, glm::vec3 position, std::uint32_t flags) {
  Particle particle;
  particle.position = position;
  particle.mass = scene.params.mass();
  particle.density = scene.params.restDensity;
  particle.flags = flags;
  scene.particles.push_back(particle);
}

void AddBlock(Scene& scene, glm::vec3 min, glm::vec3 max, std::uint32_t flags) {
  const float spacing = scene.params.particleSpacing;
  const auto steps = [spacing](float begin, float end) {
    return std::max(1, static_cast<int>(std::floor((end - begin) / spacing)));
  };
  const int nx = steps(min.x, max.x);
  const int ny = steps(min.y, max.y);
  const int nz = steps(min.z, max.z);
  scene.particles.reserve(scene.particles.size() + static_cast<std::size_t>(nx) *
                                                       static_cast<std::size_t>(ny) *
                                                       static_cast<std::size_t>(nz));
  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        const glm::vec3 position{
            min.x + (static_cast<float>(x) + 0.5F) * spacing,
            min.y + (static_cast<float>(y) + 0.5F) * spacing,
            min.z + (static_cast<float>(z) + 0.5F) * spacing,
        };
        if (position.x < max.x && position.y < max.y && position.z < max.z) {
          AddParticle(scene, position, flags);
        }
      }
    }
  }
}

void AddSphere(Scene& scene, glm::vec3 center, float radius, std::uint32_t flags) {
  const std::size_t begin = scene.particles.size();
  AddBlock(scene, center - glm::vec3(radius), center + glm::vec3(radius), flags);
  const auto first = scene.particles.begin() + static_cast<std::ptrdiff_t>(begin);
  const auto last = std::remove_if(first, scene.particles.end(), [&](const Particle& particle) {
    const glm::vec3 delta = particle.position - center;
    return glm::dot(delta, delta) > radius * radius;
  });
  scene.particles.erase(last, scene.particles.end());
}

void AddBoxBoundaries(Scene& scene) {
  const float spacing = scene.params.particleSpacing;
  const glm::vec3 min = scene.params.domainMin;
  const glm::vec3 max = scene.params.domainMax;
  const auto steps = [spacing](float begin, float end) {
    return std::max(1, static_cast<int>(std::floor((end - begin) / spacing)));
  };
  const int nx = steps(min.x, max.x);
  const int ny = steps(min.y, max.y);
  const int nz = steps(min.z, max.z);
  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        const bool shell = y == 0 || x == 0 || x == nx - 1 || z == 0 || z == nz - 1;
        if (!shell) {
          continue;
        }
        const glm::vec3 position{
            min.x + (static_cast<float>(x) + 0.5F) * spacing,
            min.y + (static_cast<float>(y) + 0.5F) * spacing,
            min.z + (static_cast<float>(z) + 0.5F) * spacing,
        };
        if (position.x < max.x && position.y < max.y && position.z < max.z) {
          AddParticle(scene, position, ParticleFlagBoundary);
        }
      }
    }
  }
}

void AddDisk(Scene& scene, float y, glm::vec2 center, float radius) {
  const float spacing = scene.params.particleSpacing;
  AddParticle(scene, {center.x, y, center.y}, ParticleFlagBoundary);
  const int rings = std::max(1, static_cast<int>(std::floor(radius / spacing)));
  for (int ring = 1; ring <= rings; ++ring) {
    const float r = static_cast<float>(ring) * spacing;
    if (r > radius) {
      break;
    }
    const int count =
        std::max(6, static_cast<int>(std::round(2.0F * std::numbers::pi_v<float> * r / spacing)));
    for (int i = 0; i < count; ++i) {
      const float ang =
          2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(count);
      AddParticle(scene, {center.x + r * std::cos(ang), y, center.y + r * std::sin(ang)},
                  ParticleFlagBoundary);
    }
  }
}

void AddCylinderBoundaries(Scene& scene) {
  const float spacing = scene.params.particleSpacing;
  const glm::vec2 center = TankCenterXz(scene.params);
  const float radius = TankRadius(scene.params);
  const glm::vec3 min = scene.params.domainMin;
  const glm::vec3 max = scene.params.domainMax;
  const int ny = std::max(1, static_cast<int>(std::floor((max.y - min.y) / spacing)));
  const int nTheta = std::max(
      8, static_cast<int>(std::round(2.0F * std::numbers::pi_v<float> * radius / spacing)));
  for (int y = 0; y < ny; ++y) {
    const float py = min.y + (static_cast<float>(y) + 0.5F) * spacing;
    if (py >= max.y) {
      continue;
    }
    for (int i = 0; i < nTheta; ++i) {
      const float ang =
          2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(nTheta);
      AddParticle(scene, {center.x + radius * std::cos(ang), py, center.y + radius * std::sin(ang)},
                  ParticleFlagBoundary);
    }
  }
  AddDisk(scene, min.y + 0.5F * spacing, center, radius);
  AddDisk(scene, max.y - 0.5F * spacing, center, radius);
}

glm::vec2 HexagonVertex(glm::vec2 center, float radius, int index) {
  const float ang = std::numbers::pi_v<float> / 6.0F +
                    std::numbers::pi_v<float> / 3.0F * static_cast<float>(index);
  return center + radius * glm::vec2(std::cos(ang), std::sin(ang));
}

void AddHexagonBoundaries(Scene& scene) {
  const float spacing = scene.params.particleSpacing;
  const glm::vec2 center = TankCenterXz(scene.params);
  const float radius = TankRadius(scene.params);
  const glm::vec3 min = scene.params.domainMin;
  const glm::vec3 max = scene.params.domainMax;
  const int ny = std::max(1, static_cast<int>(std::floor((max.y - min.y) / spacing)));
  for (int edge = 0; edge < 6; ++edge) {
    const glm::vec2 a = HexagonVertex(center, radius, edge);
    const glm::vec2 b = HexagonVertex(center, radius, edge + 1);
    const int nEdge = std::max(1, static_cast<int>(std::round(glm::length(b - a) / spacing)));
    for (int y = 0; y < ny; ++y) {
      const float py = min.y + (static_cast<float>(y) + 0.5F) * spacing;
      if (py >= max.y) {
        continue;
      }
      for (int i = 0; i < nEdge; ++i) {
        const float t = (static_cast<float>(i) + 0.5F) / static_cast<float>(nEdge);
        const glm::vec2 xz = glm::mix(a, b, t);
        AddParticle(scene, {xz.x, py, xz.y}, ParticleFlagBoundary);
      }
    }
  }
  const auto steps = [spacing](float begin, float end) {
    return std::max(1, static_cast<int>(std::floor((end - begin) / spacing)));
  };
  const int nx = steps(min.x, max.x);
  const int nz = steps(min.z, max.z);
  for (int z = 0; z < nz; ++z) {
    for (int x = 0; x < nx; ++x) {
      const glm::vec2 xz{min.x + (static_cast<float>(x) + 0.5F) * spacing,
                         min.z + (static_cast<float>(z) + 0.5F) * spacing};
      if (HexagonNorm(xz - center) > TankApothem(scene.params) + 0.25F * spacing) {
        continue;
      }
      AddParticle(scene, {xz.x, min.y + 0.5F * spacing, xz.y}, ParticleFlagBoundary);
      AddParticle(scene, {xz.x, max.y - 0.5F * spacing, xz.y}, ParticleFlagBoundary);
    }
  }
}

void AddTankBoundaries(Scene& scene) {
  switch (scene.params.tankShape) {
    case TankShape::Cylinder:
      AddCylinderBoundaries(scene);
      break;
    case TankShape::Hexagon:
      AddHexagonBoundaries(scene);
      break;
    default:
      AddBoxBoundaries(scene);
      break;
  }
}

void ClipFluidToTank(Scene& scene) {
  if (scene.params.tankShape == TankShape::Box) {
    return;
  }
  const float inset = scene.params.collisionRadius();
  const auto last =
      std::remove_if(scene.particles.begin(), scene.particles.end(), [&](const Particle& particle) {
        return particle.flags == ParticleFlagFluid &&
               !InsideTank(particle.position, scene.params, inset);
      });
  scene.particles.erase(last, scene.particles.end());
}

}  // namespace

const char* ScenePresetName(ScenePreset preset) {
  switch (preset) {
    case ScenePreset::Empty:
      return "empty";
    case ScenePreset::DamBreak:
      return "dam";
    case ScenePreset::Drop:
      return "drop";
    case ScenePreset::DoubleDam:
      return "double";
    case ScenePreset::DropIn:
      return "dropin";
  }
  return "dam";
}

ScenePreset ParseScenePreset(std::string_view name) {
  if (name == "empty") {
    return ScenePreset::Empty;
  }
  if (name == "dam" || name == "dambreak") {
    return ScenePreset::DamBreak;
  }
  if (name == "drop") {
    return ScenePreset::Drop;
  }
  if (name == "double") {
    return ScenePreset::DoubleDam;
  }
  if (name == "dropin" || name == "impact") {
    return ScenePreset::DropIn;
  }
  throw std::invalid_argument(std::format("unknown scene '{}'", name));
}

Scene BuildScene(ScenePreset preset, const SimParams& params) {
  Scene scene;
  scene.params = params;
  const float spacing = params.particleSpacing;
  const float pad = params.collisionRadius() + 0.5F * spacing;
  const glm::vec3 inset{pad, pad, pad};
  const glm::vec3 min = params.domainMin + inset;
  const glm::vec3 max = params.domainMax - inset;

  switch (preset) {
    case ScenePreset::Empty:
      break;
    case ScenePreset::DamBreak: {
      AddTankBoundaries(scene);
      glm::vec3 fluidMax = max;
      fluidMax.x = min.x + 0.42F * (max.x - min.x);
      fluidMax.y = min.y + 0.70F * (max.y - min.y);
      AddBlock(scene, min, fluidMax, ParticleFlagFluid);
      break;
    }
    case ScenePreset::Drop: {
      AddTankBoundaries(scene);
      glm::vec3 poolMax = max;
      poolMax.y = min.y + spacing * 3.0F;
      AddBlock(scene, min, poolMax, ParticleFlagFluid);
      AddSphere(scene, {(min.x + max.x) * 0.5F, min.y + 0.55F * (max.y - min.y), 0.0F}, 0.16F,
                ParticleFlagFluid);
      break;
    }
    case ScenePreset::DoubleDam: {
      AddTankBoundaries(scene);
      glm::vec3 leftMax = max;
      leftMax.x = min.x + 0.28F * (max.x - min.x);
      leftMax.y = min.y + 0.65F * (max.y - min.y);
      AddBlock(scene, min, leftMax, ParticleFlagFluid);
      glm::vec3 rightMin = min;
      rightMin.x = max.x - 0.28F * (max.x - min.x);
      glm::vec3 rightMax = max;
      rightMax.y = leftMax.y;
      AddBlock(scene, rightMin, rightMax, ParticleFlagFluid);
      break;
    }
    case ScenePreset::DropIn: {
      AddTankBoundaries(scene);
      glm::vec3 poolMax = max;
      poolMax.y = min.y + 0.34F * (max.y - min.y);
      AddBlock(scene, min, poolMax, ParticleFlagFluid);
      break;
    }
  }
  ClipFluidToTank(scene);
  CalibrateRestDensity(scene.particles, scene.params);
  CalibrateBoundaryPsi(scene);
  return scene;
}

void CalibrateBoundaryPsi(Scene& scene) {
  const float h = scene.params.supportRadius();
  const float rest = scene.params.restDensity;
  for (Particle& boundary : scene.particles) {
    if (boundary.flags != ParticleFlagBoundary) {
      continue;
    }
    float sum = 0.0F;
    for (const Particle& other : scene.particles) {
      if (other.flags != ParticleFlagBoundary) {
        continue;
      }
      const glm::vec3 delta = boundary.position - other.position;
      sum += Poly6(glm::dot(delta, delta), h);
    }
    boundary.mass = rest / std::max(sum, KernelEpsilon);
    boundary.density = rest;
  }
}

glm::vec3 WoodenBlockStart(ScenePreset preset, const SimParams& params) {
  const float floorY = params.domainMin.y + WoodenBlockHalf.y + params.collisionRadius();
  glm::vec3 center =
      preset == ScenePreset::DropIn
          ? glm::vec3{0.0F, params.domainMax.y - WoodenBlockHalf.y - params.collisionRadius(), 0.0F}
          : glm::vec3{0.32F, floorY, 0.0F};
  ClampAabbToTank(center, WoodenBlockHalf, params);
  return center;
}

void AdvanceWoodenBlock(glm::vec3& center, glm::vec3& velocity, const SimParams& params) {
  const float dt = params.timeStep * static_cast<float>(std::max(1U, params.substeps));
  velocity += params.gravity * dt;
  center += velocity * dt;
  const glm::vec3 previous = center;
  ClampAabbToTank(center, WoodenBlockHalf, params);
  const glm::vec3 delta = center - previous;
  for (int axis = 0; axis < 3; ++axis) {
    if (delta[axis] != 0.0F && velocity[axis] * delta[axis] < 0.0F) {
      velocity[axis] *= -params.restitution;
    }
  }
}

}  // namespace sph::sim
