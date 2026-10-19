#include "sim/Scene.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <format>
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

void AddFloor(Scene& scene) {
  const float spacing = scene.params.particleSpacing;
  const glm::vec3 min = scene.params.domainMin;
  glm::vec3 max = scene.params.domainMax;
  max.y = min.y + spacing;
  AddBlock(scene, min, max, ParticleFlagBoundary);
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
  throw std::invalid_argument(std::format("unknown scene '{}'", name));
}

Scene BuildScene(ScenePreset preset, const SimParams& params) {
  Scene scene;
  scene.params = params;
  const float spacing = params.particleSpacing;
  const glm::vec3 inset{spacing * 2.0F, spacing, spacing * 2.0F};
  const glm::vec3 min = params.domainMin + inset;
  const glm::vec3 max = params.domainMax - inset;

  switch (preset) {
    case ScenePreset::Empty:
      break;
    case ScenePreset::DamBreak: {
      AddFloor(scene);
      glm::vec3 fluidMax = max;
      fluidMax.x = min.x + 0.42F * (max.x - min.x);
      fluidMax.y = min.y + 0.70F * (max.y - min.y);
      AddBlock(scene, min, fluidMax, ParticleFlagFluid);
      break;
    }
    case ScenePreset::Drop: {
      AddFloor(scene);
      glm::vec3 poolMax = max;
      poolMax.y = min.y + spacing * 3.0F;
      AddBlock(scene, min, poolMax, ParticleFlagFluid);
      AddSphere(scene, {(min.x + max.x) * 0.5F, min.y + 0.55F * (max.y - min.y), 0.0F}, 0.16F,
                ParticleFlagFluid);
      break;
    }
    case ScenePreset::DoubleDam: {
      AddFloor(scene);
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
  }
  return scene;
}

}  // namespace sph::sim
