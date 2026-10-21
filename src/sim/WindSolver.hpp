#pragma once

#include "gpu/Buffer.hpp"
#include "gpu/Handles.hpp"
#include "sim/Car.hpp"
#include "sim/Particle.hpp"

#include <glm/vec4.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace sph {
namespace gpu {
class Context;
}
namespace sim {

struct WindAero {
  float drag = 0.0F;
  float lift = 0.0F;
  float side = 0.0F;
  float cd = 0.0F;
  float cl = 0.0F;
  float frontalArea = 0.0F;
};

class WindSolver {
 public:
  static constexpr std::uint32_t TracerCount = 8192;

  explicit WindSolver(const gpu::Context& context);

  void reset();
  void record(VkCommandBuffer commandBuffer, std::uint32_t steps = 1);
  void step(std::uint32_t steps = 1);
  void downloadMacro(std::span<glm::vec4> dest) const;
  void downloadTracers(std::span<Particle> dest) const;
  void setObstacleMesh(const std::filesystem::path& path);
  void useFormulaCar();

  [[nodiscard]] VkDeviceAddress meshAddress() const { return mesh_.address(); }
  [[nodiscard]] std::uint32_t meshVertexCount() const { return meshCount_; }
  [[nodiscard]] VkDeviceAddress tracerAddress() const { return tracers_.address(); }
  [[nodiscard]] std::uint32_t tracerCount() const { return TracerCount; }
  [[nodiscard]] VkDeviceAddress macroAddress() const { return macro_.address(); }
  [[nodiscard]] VkDeviceAddress occupancyAddress() const { return occupancy_.address(); }
  [[nodiscard]] const CarDomain& domain() const { return domain_; }
  [[nodiscard]] glm::vec3 domainMin() const { return domain_.origin; }
  [[nodiscard]] glm::vec3 domainMax() const { return CarDomainMax(domain_); }
  [[nodiscard]] float inletSpeed() const { return inletUx_; }
  [[nodiscard]] const WindAero& aero() const { return aero_; }
  void setInletSpeed(float latticeSpeed);
  void refreshAero();

 private:
  void push(VkCommandBuffer commandBuffer, bool collide) const;
  void initState();
  void uploadMesh();
  void cacheFrontalArea(std::span<const std::uint32_t> occupancy);

  const gpu::Context* context_ = nullptr;
  CarDomain domain_{};
  float tau_ = 0.60F;
  float inletUx_ = 0.08F;
  std::uint32_t cellCount_ = 0;
  std::uint32_t meshCount_ = 0;
  mutable std::uint32_t seed_ = 1;
  gpu::Unique<VkPipelineLayout> layout_;
  gpu::Unique<VkPipeline> collide_;
  gpu::Unique<VkPipeline> stream_;
  gpu::Unique<VkPipeline> tracersCompute_;
  gpu::Buffer current_;
  gpu::Buffer scratch_;
  gpu::Buffer occupancy_;
  gpu::Buffer macro_;
  gpu::Buffer tracers_;
  gpu::Buffer mesh_;
  std::vector<MeshVertex> cpuMesh_;
  bool importedMesh_ = false;
  float frontalArea_ = 0.0F;
  WindAero aero_{};
};

}  // namespace sim
}  // namespace sph
