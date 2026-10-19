#pragma once

#include "gpu/Buffer.hpp"
#include "gpu/Handles.hpp"
#include "sim/Particle.hpp"
#include "sim/SimParams.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace sph {
namespace gpu {
class Context;
}
namespace sim {

class SphSolver {
 public:
  explicit SphSolver(const gpu::Context& context);

  void upload(std::span<const Particle> particles, const SimParams& params);
  void setCursor(glm::vec3 position, glm::vec3 force, float radius);
  void setViscosity(float viscosity);
  void record(VkCommandBuffer commandBuffer, std::uint32_t substeps) const;
  void step(std::uint32_t substeps);
  void download(std::span<Particle> particles) const;

  [[nodiscard]] VkDeviceAddress particleAddress() const { return particles_.address(); }
  [[nodiscard]] std::uint32_t count() const { return count_; }
  [[nodiscard]] const SimParams& params() const { return paramsHost_; }

  [[nodiscard]] std::vector<std::uint32_t> exclusiveScan(std::span<const std::uint32_t> counts);

 private:
  void ensureParticleBuffers(std::uint32_t particleCount);
  void ensureGridBuffers(std::uint32_t cellCount);
  void writeParams(std::uint32_t particleCount, std::uint32_t cellCount);
  void push(VkCommandBuffer commandBuffer) const;
  void recordScan(VkCommandBuffer commandBuffer) const;
  void recordRebuild(VkCommandBuffer commandBuffer) const;
  void recordSubstep(VkCommandBuffer commandBuffer) const;
  void dispatchParticles(VkCommandBuffer commandBuffer, VkPipeline pipeline) const;
  void dispatchCells(VkCommandBuffer commandBuffer, VkPipeline pipeline) const;

  const gpu::Context* context_ = nullptr;
  SimParams paramsHost_{};
  glm::vec3 cursorPosition_{0.0F};
  glm::vec3 cursorForce_{0.0F};
  float cursorRadius_ = 0.0F;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> hashCount_;
  gpu::Unique<VkPipeline> scan_;
  gpu::Unique<VkPipeline> scanBlocks_;
  gpu::Unique<VkPipeline> scanAdd_;
  gpu::Unique<VkPipeline> scatter_;
  gpu::Unique<VkPipeline> density_;
  gpu::Unique<VkPipeline> forces_;
  gpu::Unique<VkPipeline> integrate_;
  gpu::Buffer params_;
  gpu::Buffer particles_;
  gpu::Buffer sorted_;
  gpu::Buffer cellOf_;
  gpu::Buffer dest_;
  gpu::Buffer counts_;
  gpu::Buffer offsets_;
  gpu::Buffer fill_;
  gpu::Buffer blockSums_;
  std::uint32_t count_ = 0;
  std::uint32_t cellCount_ = 0;
  std::uint32_t particleCapacity_ = 0;
  std::uint32_t cellCapacity_ = 0;
};

}  // namespace sim
}  // namespace sph
