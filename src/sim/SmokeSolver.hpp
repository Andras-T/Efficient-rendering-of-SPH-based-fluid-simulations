#pragma once

#include "gpu/Buffer.hpp"
#include "gpu/Handles.hpp"

#include <glm/ext/vector_uint3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <span>

namespace sph {
namespace gpu {
class Context;
}
namespace sim {

struct SmokeDomain {
  glm::vec3 origin{-0.80F, 0.0F, -0.80F};
  float cellSize = 0.05F;
  glm::uvec3 grid{32, 48, 32};
};

[[nodiscard]] constexpr glm::vec3 SmokeDomainMax(const SmokeDomain& domain) {
  return domain.origin + domain.cellSize * glm::vec3(domain.grid);
}

class SmokeSolver {
 public:
  static constexpr std::uint32_t JacobiIterations = 8;

  explicit SmokeSolver(const gpu::Context& context);

  void reset();
  void record(VkCommandBuffer commandBuffer, std::uint32_t steps = 1);
  void step(std::uint32_t steps = 1);
  void downloadScalar(std::span<glm::vec2> dest) const;
  void downloadVelocity(std::span<glm::vec4> dest) const;

  [[nodiscard]] VkDeviceAddress scalarAddress() const { return scalarFront().address(); }
  [[nodiscard]] VkDeviceAddress velocityAddress() const { return velocityFront().address(); }
  [[nodiscard]] const SmokeDomain& domain() const { return domain_; }
  [[nodiscard]] glm::vec3 domainMin() const { return domain_.origin; }
  [[nodiscard]] glm::vec3 domainMax() const { return SmokeDomainMax(domain_); }
  [[nodiscard]] std::uint32_t cellCount() const { return cellCount_; }
  [[nodiscard]] float emitterStrength() const { return emitterStrength_; }
  void setEmitterStrength(float strength);

 private:
  void push(VkCommandBuffer commandBuffer, VkDeviceAddress velocitySrc, VkDeviceAddress velocityDst,
            VkDeviceAddress scalarSrc, VkDeviceAddress scalarDst, VkDeviceAddress pressureSrc,
            VkDeviceAddress pressureDst) const;
  void initState();
  [[nodiscard]] const gpu::Buffer& velocityFront() const {
    return velocityFlip_ ? velocityB_ : velocityA_;
  }
  [[nodiscard]] gpu::Buffer& velocityFront() { return velocityFlip_ ? velocityB_ : velocityA_; }
  [[nodiscard]] gpu::Buffer& velocityBack() { return velocityFlip_ ? velocityA_ : velocityB_; }
  [[nodiscard]] const gpu::Buffer& scalarFront() const { return scalarFlip_ ? scalarB_ : scalarA_; }
  [[nodiscard]] gpu::Buffer& scalarFront() { return scalarFlip_ ? scalarB_ : scalarA_; }
  [[nodiscard]] gpu::Buffer& scalarBack() { return scalarFlip_ ? scalarA_ : scalarB_; }

  const gpu::Context* context_ = nullptr;
  SmokeDomain domain_{};
  std::uint32_t cellCount_ = 0;
  float dt_ = 0.04F;
  float dissipation_ = 0.988F;
  float buoyancy_ = 4.4F;
  float vorticity_ = 1.8F;
  float emitterStrength_ = 1.0F;
  glm::vec3 emitterCenter_{0.0F, 0.14F, 0.0F};
  float emitterRadius_ = 0.13F;
  bool velocityFlip_ = false;
  bool scalarFlip_ = false;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> advectVel_;
  gpu::Unique<VkPipeline> forces_;
  gpu::Unique<VkPipeline> divergence_;
  gpu::Unique<VkPipeline> jacobi_;
  gpu::Unique<VkPipeline> project_;
  gpu::Unique<VkPipeline> advectScalar_;
  gpu::Buffer velocityA_;
  gpu::Buffer velocityB_;
  gpu::Buffer scalarA_;
  gpu::Buffer scalarB_;
  gpu::Buffer pressureA_;
  gpu::Buffer pressureB_;
  gpu::Buffer divergenceBuf_;
};

}  // namespace sim
}  // namespace sph
