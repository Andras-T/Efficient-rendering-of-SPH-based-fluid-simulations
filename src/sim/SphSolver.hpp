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
  void setCursor(glm::vec3 position, glm::vec3 force, float radius, float swirl = 0.0F,
                 float push = 0.0F);
  void setBlock(glm::vec3 min, glm::vec3 max, bool enabled);
  void setViscosity(float viscosity);
  void setSolver(SolverMethod solver);
  void record(VkCommandBuffer commandBuffer, std::uint32_t substeps) const;
  void recordBlockSample(VkCommandBuffer commandBuffer) const;
  void step(std::uint32_t substeps);
  void download(std::span<Particle> particles) const;
  [[nodiscard]] float blockWaterSurface() const;

  [[nodiscard]] VkDeviceAddress particleAddress() const { return particles_.address(); }
  [[nodiscard]] VkDeviceAddress foamAddress() const { return foam_.address(); }
  [[nodiscard]] std::uint32_t count() const { return count_; }
  [[nodiscard]] std::uint32_t foamCapacity() const { return FoamCapacity; }
  [[nodiscard]] const SimParams& params() const { return paramsHost_; }

  [[nodiscard]] std::vector<std::uint32_t> exclusiveScan(std::span<const std::uint32_t> counts);

 private:
  void ensureParticleBuffers(std::uint32_t particleCount);
  void ensureGridBuffers(std::uint32_t cellCount);
  void writeParams(std::uint32_t particleCount, std::uint32_t cellCount);
  void push(VkCommandBuffer commandBuffer, std::uint32_t solvePass = 0) const;
  void recordScan(VkCommandBuffer commandBuffer) const;
  void recordRebuild(VkCommandBuffer commandBuffer) const;
  void recordSubstep(VkCommandBuffer commandBuffer) const;
  void recordFoam(VkCommandBuffer commandBuffer) const;
  void dispatchParticles(VkCommandBuffer commandBuffer, VkPipeline pipeline,
                         std::uint32_t solvePass = 0) const;
  void dispatchCells(VkCommandBuffer commandBuffer, VkPipeline pipeline) const;
  void dispatchFoam(VkCommandBuffer commandBuffer, VkPipeline pipeline) const;

  const gpu::Context* context_ = nullptr;
  SimParams paramsHost_{};
  glm::vec3 cursorPosition_{0.0F};
  glm::vec3 cursorForce_{0.0F};
  float cursorRadius_ = 0.0F;
  float cursorSwirl_ = 0.0F;
  float cursorPush_ = 0.0F;
  glm::vec3 blockMin_{0.0F};
  glm::vec3 blockMax_{0.0F};
  bool blockEnabled_ = false;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> hashCount_;
  gpu::Unique<VkPipeline> scan_;
  gpu::Unique<VkPipeline> scanBlocks_;
  gpu::Unique<VkPipeline> scanAdd_;
  gpu::Unique<VkPipeline> scatter_;
  gpu::Unique<VkPipeline> density_;
  gpu::Unique<VkPipeline> forces_;
  gpu::Unique<VkPipeline> integrate_;
  gpu::Unique<VkPipeline> kick_;
  gpu::Unique<VkPipeline> drift_;
  gpu::Unique<VkPipeline> alpha_;
  gpu::Unique<VkPipeline> kappa_;
  gpu::Unique<VkPipeline> apply_;
  gpu::Unique<VkPipeline> foamSpawn_;
  gpu::Unique<VkPipeline> foamAdvect_;
  gpu::Unique<VkPipeline> blockSample_;
  gpu::Buffer params_;
  gpu::Buffer sample_;
  gpu::Buffer particles_;
  gpu::Buffer sorted_;
  gpu::Buffer cellOf_;
  gpu::Buffer dest_;
  gpu::Buffer counts_;
  gpu::Buffer offsets_;
  gpu::Buffer fill_;
  gpu::Buffer blockSums_;
  gpu::Buffer foam_;
  std::uint32_t count_ = 0;
  std::uint32_t cellCount_ = 0;
  std::uint32_t particleCapacity_ = 0;
  std::uint32_t cellCapacity_ = 0;
  mutable std::uint32_t foamSeed_ = 1;
};

}  // namespace sim
}  // namespace sph
