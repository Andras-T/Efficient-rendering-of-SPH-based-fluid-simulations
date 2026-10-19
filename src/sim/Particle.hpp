#pragma once

#include "gpu/Vulkan.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>

namespace sph::sim {

constexpr std::uint32_t ParticleFlagFluid = 0;
constexpr std::uint32_t ParticleFlagBoundary = 1;

// Scalar-layout particle; must match shaders/Types.glsl.
struct Particle {
  glm::vec3 position{0.0F};
  float mass = 0.0F;
  glm::vec3 velocity{0.0F};
  float density = 0.0F;
  glm::vec3 force{0.0F};
  float pressure = 0.0F;
  std::uint32_t flags = ParticleFlagFluid;
  float pad0 = 0.0F;
  float pad1 = 0.0F;
  float pad2 = 0.0F;
};

static_assert(sizeof(Particle) == 64);

struct FrameUniforms {
  glm::mat4 view{1.0F};
  glm::mat4 projection{1.0F};
  glm::vec3 lightDirection{0.35F, 0.85F, 0.40F};
  float radius = 0.02F;
  VkDeviceAddress particles = 0;
  std::uint32_t count = 0;
  std::uint32_t pad = 0;
};

static_assert(sizeof(FrameUniforms) == 160);

}  // namespace sph::sim
