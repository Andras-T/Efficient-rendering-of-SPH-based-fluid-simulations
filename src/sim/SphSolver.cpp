#include "sim/SphSolver.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"
#include "sim/Grid.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace sph::sim {
namespace {

constexpr std::uint32_t SphGroupSize = 64;
constexpr std::uint32_t ScanGroupSize = 256;
constexpr std::uint32_t MaxScanBlocks = 256;

struct SphPush {
  VkDeviceAddress particles = 0;
  VkDeviceAddress sorted = 0;
  VkDeviceAddress cellOf = 0;
  VkDeviceAddress dest = 0;
  VkDeviceAddress counts = 0;
  VkDeviceAddress offsets = 0;
  VkDeviceAddress fill = 0;
  VkDeviceAddress blockSums = 0;
  VkDeviceAddress params = 0;
};

static_assert(sizeof(SphPush) == 72);

constexpr VkBufferUsageFlags StorageUsage =
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

constexpr VmaAllocationCreateFlags HostAccess =
    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

[[nodiscard]] std::uint32_t DispatchCount(std::uint32_t items, std::uint32_t groupSize) {
  return items == 0 ? 0 : (items + groupSize - 1) / groupSize;
}

gpu::Buffer MakeStorage(const gpu::Context& context, VkDeviceSize size) {
  return gpu::Buffer(context, {.size = size, .usage = StorageUsage, .allocationFlags = HostAccess});
}

gpu::Unique<VkPipeline> MakeCompute(VkDevice device, VkPipelineLayout layout,
                                    std::string_view name) {
  return gpu::CreateComputePipeline(device, {.spirv = shaders::Find(name), .layout = layout});
}

GpuSimParams MakeGpuParams(const SimParams& params, const Grid& grid, std::uint32_t particleCount,
                           float mass, glm::vec3 cursorPosition, glm::vec3 cursorForce,
                           float cursorRadius) {
  const float kernelRadius = params.supportRadius();
  return GpuSimParams{
      .domainMin = params.domainMin,
      .kernelRadius = kernelRadius,
      .domainMax = params.domainMax,
      .kernelRadius2 = kernelRadius * kernelRadius,
      .gravity = params.gravity,
      .mass = mass,
      .restDensity = params.restDensity,
      .stiffness = params.stiffness(),
      .eosGamma = params.eosGamma,
      .viscosity = params.viscosity,
      .dt = params.timeStep,
      .restitution = params.restitution,
      .collisionRadius = params.particleSpacing * 0.5F,
      .invCellSize = 1.0F / grid.cellSize,
      .gridSize = grid.size,
      .particleCount = particleCount,
      .cellCount = grid.cellCount,
      .cursorRadius = cursorRadius,
      .cursorPosition = cursorPosition,
      .cursorForce = cursorForce,
  };
}

void ComputeToCompute(VkCommandBuffer commandBuffer) {
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
}

}  // namespace

SphSolver::SphSolver(const gpu::Context& context) : context_(&context) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .size = sizeof(SphPush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const VkDevice device = context.device();
  const VkPipelineLayout layout = layout_.get();
  hashCount_ = MakeCompute(device, layout, "HashCount.comp");
  scan_ = MakeCompute(device, layout, "Scan.comp");
  scanBlocks_ = MakeCompute(device, layout, "ScanBlocks.comp");
  scanAdd_ = MakeCompute(device, layout, "ScanAdd.comp");
  scatter_ = MakeCompute(device, layout, "Scatter.comp");
  density_ = MakeCompute(device, layout, "Density.comp");
  forces_ = MakeCompute(device, layout, "Forces.comp");
  integrate_ = MakeCompute(device, layout, "Integrate.comp");
  params_ = MakeStorage(context, sizeof(GpuSimParams));
}

void SphSolver::ensureParticleBuffers(std::uint32_t particleCount) {
  if (particleCount <= particleCapacity_ && particles_.handle() != VK_NULL_HANDLE) {
    return;
  }
  if (particleCount == 0) {
    particles_ = {};
    sorted_ = {};
    cellOf_ = {};
    dest_ = {};
    particleCapacity_ = 0;
    return;
  }
  const auto count = static_cast<VkDeviceSize>(particleCount);
  particles_ = MakeStorage(*context_, count * sizeof(Particle));
  sorted_ = MakeStorage(*context_, count * sizeof(Particle));
  cellOf_ = MakeStorage(*context_, count * sizeof(std::uint32_t));
  dest_ = MakeStorage(*context_, count * sizeof(std::uint32_t));
  particleCapacity_ = particleCount;
}

void SphSolver::ensureGridBuffers(std::uint32_t cellCount) {
  const std::uint32_t blocks = DispatchCount(cellCount, ScanGroupSize);
  if (blocks > MaxScanBlocks) {
    throw std::runtime_error("uniform grid exceeds the two-level scan capacity");
  }
  if (cellCount <= cellCapacity_ && counts_.handle() != VK_NULL_HANDLE) {
    return;
  }
  const auto cells = static_cast<VkDeviceSize>(std::max(cellCount, 1U));
  const auto blockCount = static_cast<VkDeviceSize>(std::max(blocks, 1U));
  counts_ = MakeStorage(*context_, cells * sizeof(std::uint32_t));
  offsets_ = MakeStorage(*context_, cells * sizeof(std::uint32_t));
  fill_ = MakeStorage(*context_, cells * sizeof(std::uint32_t));
  blockSums_ = MakeStorage(*context_, blockCount * sizeof(std::uint32_t));
  cellCapacity_ = cellCount;
}

void SphSolver::writeParams(std::uint32_t particleCount, std::uint32_t cellCount) {
  const Grid grid = MakeGrid(paramsHost_);
  const float mass =
      particleCount == 0 ? paramsHost_.mass() : particles_.mapped<Particle>().front().mass;
  GpuSimParams gpu = MakeGpuParams(paramsHost_, grid, particleCount, mass, cursorPosition_,
                                   cursorForce_, cursorRadius_);
  gpu.cellCount = cellCount;
  gpu.particleCount = particleCount;
  params_.mapped<GpuSimParams>().front() = gpu;
  params_.flush();
}

void SphSolver::setCursor(glm::vec3 position, glm::vec3 force, float radius) {
  cursorPosition_ = position;
  cursorForce_ = force;
  cursorRadius_ = radius;
  if (params_.handle() != VK_NULL_HANDLE) {
    writeParams(count_, cellCount_);
  }
}

void SphSolver::setViscosity(float viscosity) {
  paramsHost_.viscosity = viscosity;
  if (params_.handle() != VK_NULL_HANDLE) {
    writeParams(count_, cellCount_);
  }
}

void SphSolver::push(VkCommandBuffer commandBuffer) const {
  const SphPush constants{
      .particles = particles_.address(),
      .sorted = sorted_.address(),
      .cellOf = cellOf_.address(),
      .dest = dest_.address(),
      .counts = counts_.address(),
      .offsets = offsets_.address(),
      .fill = fill_.address(),
      .blockSums = blockSums_.address(),
      .params = params_.address(),
  };
  vkCmdPushConstants(commandBuffer, layout_.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(constants), &constants);
}

void SphSolver::dispatchParticles(VkCommandBuffer commandBuffer, VkPipeline pipeline) const {
  const std::uint32_t groups = DispatchCount(count_, SphGroupSize);
  if (groups == 0) {
    return;
  }
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  push(commandBuffer);
  vkCmdDispatch(commandBuffer, groups, 1, 1);
}

void SphSolver::dispatchCells(VkCommandBuffer commandBuffer, VkPipeline pipeline) const {
  const std::uint32_t groups = DispatchCount(cellCount_, ScanGroupSize);
  if (groups == 0) {
    return;
  }
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  push(commandBuffer);
  vkCmdDispatch(commandBuffer, groups, 1, 1);
}

void SphSolver::recordScan(VkCommandBuffer commandBuffer) const {
  dispatchCells(commandBuffer, scan_.get());
  ComputeToCompute(commandBuffer);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, scanBlocks_.get());
  push(commandBuffer);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ComputeToCompute(commandBuffer);
  dispatchCells(commandBuffer, scanAdd_.get());
}

void SphSolver::recordRebuild(VkCommandBuffer commandBuffer) const {
  const VkDeviceSize bytes = static_cast<VkDeviceSize>(cellCount_) * sizeof(std::uint32_t);
  vkCmdFillBuffer(commandBuffer, counts_.handle(), 0, bytes, 0);
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

  dispatchParticles(commandBuffer, hashCount_.get());
  ComputeToCompute(commandBuffer);
  recordScan(commandBuffer);
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT,
                     VK_ACCESS_2_TRANSFER_READ_BIT);

  const VkBufferCopy cellCopy{.size = bytes};
  vkCmdCopyBuffer(commandBuffer, offsets_.handle(), fill_.handle(), 1, &cellCopy);
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

  dispatchParticles(commandBuffer, scatter_.get());
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT,
                     VK_ACCESS_2_TRANSFER_READ_BIT);

  const VkBufferCopy particleCopy{.size = static_cast<VkDeviceSize>(count_) * sizeof(Particle)};
  vkCmdCopyBuffer(commandBuffer, sorted_.handle(), particles_.handle(), 1, &particleCopy);
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
}

void SphSolver::recordSubstep(VkCommandBuffer commandBuffer) const {
  recordRebuild(commandBuffer);
  dispatchParticles(commandBuffer, density_.get());
  ComputeToCompute(commandBuffer);
  dispatchParticles(commandBuffer, forces_.get());
  ComputeToCompute(commandBuffer);
  dispatchParticles(commandBuffer, integrate_.get());
  ComputeToCompute(commandBuffer);
}

void SphSolver::upload(std::span<const Particle> particles, const SimParams& params) {
  paramsHost_ = params;
  count_ = static_cast<std::uint32_t>(particles.size());
  const Grid grid = MakeGrid(paramsHost_);
  cellCount_ = grid.cellCount;
  ensureParticleBuffers(count_);
  ensureGridBuffers(cellCount_);
  if (count_ > 0) {
    std::memcpy(particles_.mapped<std::uint8_t>().data(), particles.data(), particles.size_bytes());
    particles_.flush();
  }
  writeParams(count_, cellCount_);
}

void SphSolver::record(VkCommandBuffer commandBuffer, std::uint32_t substeps) const {
  if (count_ == 0 || substeps == 0) {
    return;
  }
  for (std::uint32_t step = 0; step < substeps; ++step) {
    recordSubstep(commandBuffer);
  }
}

void SphSolver::step(std::uint32_t substeps) {
  gpu::ImmediateSubmit(*context_, [&](VkCommandBuffer commandBuffer) {
    record(commandBuffer, substeps);
    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_HOST_BIT,
                       VK_ACCESS_2_HOST_READ_BIT);
  });
}

void SphSolver::download(std::span<Particle> particles) const {
  if (particles.size() != count_) {
    throw std::runtime_error("particle download size does not match the solver");
  }
  if (count_ == 0) {
    return;
  }
  context_->waitIdle();
  particles_.invalidate();
  std::memcpy(particles.data(), particles_.mapped<const std::uint8_t>().data(),
              particles.size_bytes());
}

std::vector<std::uint32_t> SphSolver::exclusiveScan(std::span<const std::uint32_t> counts) {
  if (counts.empty()) {
    return {};
  }
  const auto n = static_cast<std::uint32_t>(counts.size());
  const std::uint32_t savedCount = count_;
  const std::uint32_t savedCells = cellCount_;
  ensureGridBuffers(n);
  cellCount_ = n;
  std::memcpy(counts_.mapped<std::uint8_t>().data(), counts.data(), counts.size_bytes());
  counts_.flush();
  writeParams(0, n);
  gpu::ImmediateSubmit(*context_, [&](VkCommandBuffer commandBuffer) {
    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_WRITE_BIT,
                       VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
    recordScan(commandBuffer);
    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_HOST_BIT,
                       VK_ACCESS_2_HOST_READ_BIT);
  });
  offsets_.invalidate();
  const auto mapped = offsets_.mapped<const std::uint32_t>();
  std::vector<std::uint32_t> result(mapped.begin(),
                                    mapped.begin() + static_cast<std::ptrdiff_t>(n));
  count_ = savedCount;
  cellCount_ = savedCells;
  if (savedCells > 0) {
    writeParams(savedCount, savedCells);
  }
  return result;
}

}  // namespace sph::sim
