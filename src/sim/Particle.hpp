#pragma once

#include "gpu/Vulkan.hpp"

#include <glm/ext/vector_uint3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
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

struct GpuSimParams {
  glm::vec3 domainMin{0.0F};
  float kernelRadius = 0.0F;
  glm::vec3 domainMax{0.0F};
  float kernelRadius2 = 0.0F;
  glm::vec3 gravity{0.0F};
  float mass = 0.0F;
  float restDensity = 0.0F;
  float stiffness = 0.0F;
  float eosGamma = 0.0F;
  float viscosity = 0.0F;
  float dt = 0.0F;
  float restitution = 0.0F;
  float collisionRadius = 0.0F;
  float invCellSize = 0.0F;
  glm::uvec3 gridSize{1, 1, 1};
  std::uint32_t particleCount = 0;
  std::uint32_t cellCount = 0;
  std::uint32_t pad0 = 0;
  std::uint32_t pad1 = 0;
  std::uint32_t pad2 = 0;
};

static_assert(sizeof(GpuSimParams) == 112);

struct FrameUniforms {
  glm::mat4 view{1.0F};
  glm::mat4 projection{1.0F};
  glm::mat4 inverseView{1.0F};
  glm::mat4 inverseProjection{1.0F};
  glm::vec3 lightDirection{0.35F, 0.85F, 0.40F};
  float radius = 0.02F;
  VkDeviceAddress particles = 0;
  std::uint32_t count = 0;
  std::uint32_t debugView = 0;
  glm::vec3 absorption{2.45F, 0.42F, 0.12F};
  float ior = 1.333F;
  glm::vec3 domainMin{0.0F};
  float restDensity = 1000.0F;
  glm::vec3 domainMax{1.0F};
  float invCellSize = 1.0F;
  glm::uvec3 gridSize{1, 1, 1};
  float exposure = 0.85F;
};

static_assert(offsetof(FrameUniforms, inverseView) == 128);
static_assert(offsetof(FrameUniforms, particles) == 272);
static_assert(offsetof(FrameUniforms, absorption) == 288);
static_assert(offsetof(FrameUniforms, exposure) == 348);
static_assert(sizeof(FrameUniforms) == 352);

}  // namespace sph::sim
