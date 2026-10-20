#include "sim/SmokeSolver.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>
#include <vector>

namespace sph::sim {
namespace {

constexpr std::uint32_t SmokeGroupSize = 64;

struct SmokePush {
  VkDeviceAddress velocitySrc = 0;
  VkDeviceAddress velocityDst = 0;
  VkDeviceAddress scalarSrc = 0;
  VkDeviceAddress scalarDst = 0;
  VkDeviceAddress pressureSrc = 0;
  VkDeviceAddress pressureDst = 0;
  VkDeviceAddress divergence = 0;
  glm::uvec3 grid{1, 1, 1};
  std::uint32_t cellCount = 0;
  glm::vec3 origin{0.0F};
  float cellSize = 0.05F;
  float dt = 0.04F;
  float dissipation = 0.988F;
  float buoyancy = 4.4F;
  float vorticity = 1.8F;
  glm::vec3 emitterCenter{0.0F};
  float emitterRadius = 0.13F;
  float emitterDensity = 1.4F;
  float emitterTemperature = 1.15F;
};

static_assert(sizeof(SmokePush) == 128);

constexpr VkBufferUsageFlags StorageUsage =
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

constexpr VmaAllocationCreateFlags HostAccess =
    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

gpu::Buffer MakeStorage(const gpu::Context& context, VkDeviceSize size) {
  return gpu::Buffer(context, {.size = size, .usage = StorageUsage, .allocationFlags = HostAccess});
}

gpu::Unique<VkPipeline> MakeCompute(VkDevice device, VkPipelineLayout layout,
                                    std::string_view name) {
  return gpu::CreateComputePipeline(device, {.spirv = shaders::Find(name), .layout = layout});
}

void ComputeBarrier(VkCommandBuffer commandBuffer) {
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
}

}  // namespace

SmokeSolver::SmokeSolver(const gpu::Context& context)
    : context_(&context), cellCount_(domain_.grid.x * domain_.grid.y * domain_.grid.z) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .size = sizeof(SmokePush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const VkDevice device = context.device();
  advectVel_ = MakeCompute(device, layout_.get(), "SmokeAdvectVel.comp");
  forces_ = MakeCompute(device, layout_.get(), "SmokeForces.comp");
  divergence_ = MakeCompute(device, layout_.get(), "SmokeDivergence.comp");
  jacobi_ = MakeCompute(device, layout_.get(), "SmokeJacobi.comp");
  project_ = MakeCompute(device, layout_.get(), "SmokeProject.comp");
  advectScalar_ = MakeCompute(device, layout_.get(), "SmokeAdvectScalar.comp");
  const VkDeviceSize velocityBytes = static_cast<VkDeviceSize>(cellCount_) * sizeof(glm::vec4);
  const VkDeviceSize scalarBytes = static_cast<VkDeviceSize>(cellCount_) * sizeof(glm::vec2);
  const VkDeviceSize pressureBytes = static_cast<VkDeviceSize>(cellCount_) * sizeof(float);
  velocityA_ = MakeStorage(context, velocityBytes);
  velocityB_ = MakeStorage(context, velocityBytes);
  scalarA_ = MakeStorage(context, scalarBytes);
  scalarB_ = MakeStorage(context, scalarBytes);
  pressureA_ = MakeStorage(context, pressureBytes);
  pressureB_ = MakeStorage(context, pressureBytes);
  divergenceBuf_ = MakeStorage(context, pressureBytes);
  initState();
}

void SmokeSolver::setEmitterStrength(float strength) {
  emitterStrength_ = std::clamp(strength, 0.35F, 2.2F);
}

void SmokeSolver::initState() {
  const auto zero = [](gpu::Buffer& buffer) {
    std::memset(buffer.mapped<std::uint8_t>().data(), 0, buffer.size());
    buffer.flush();
  };
  zero(velocityA_);
  zero(velocityB_);
  zero(scalarA_);
  zero(scalarB_);
  zero(pressureA_);
  zero(pressureB_);
  zero(divergenceBuf_);
  velocityFlip_ = false;
  scalarFlip_ = false;
}

void SmokeSolver::reset() { initState(); }

void SmokeSolver::push(VkCommandBuffer commandBuffer, VkDeviceAddress velocitySrc,
                       VkDeviceAddress velocityDst, VkDeviceAddress scalarSrc,
                       VkDeviceAddress scalarDst, VkDeviceAddress pressureSrc,
                       VkDeviceAddress pressureDst) const {
  const SmokePush constants{
      .velocitySrc = velocitySrc,
      .velocityDst = velocityDst,
      .scalarSrc = scalarSrc,
      .scalarDst = scalarDst,
      .pressureSrc = pressureSrc,
      .pressureDst = pressureDst,
      .divergence = divergenceBuf_.address(),
      .grid = domain_.grid,
      .cellCount = cellCount_,
      .origin = domain_.origin,
      .cellSize = domain_.cellSize,
      .dt = dt_,
      .dissipation = dissipation_,
      .buoyancy = buoyancy_,
      .vorticity = vorticity_,
      .emitterCenter = emitterCenter_,
      .emitterRadius = emitterRadius_,
      .emitterDensity = 1.45F * emitterStrength_,
      .emitterTemperature = 1.20F * emitterStrength_,
  };
  vkCmdPushConstants(commandBuffer, layout_.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(constants), &constants);
}

void SmokeSolver::record(VkCommandBuffer commandBuffer, std::uint32_t steps) {
  const std::uint32_t groups = (cellCount_ + SmokeGroupSize - 1) / SmokeGroupSize;
  for (std::uint32_t step = 0; step < steps; ++step) {
    const gpu::Buffer& velSrc = velocityFront();
    const gpu::Buffer& velDst = velocityBack();
    const gpu::Buffer& scalarSrc = scalarFront();
    const gpu::Buffer& scalarDst = scalarBack();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, advectVel_.get());
    push(commandBuffer, velSrc.address(), velDst.address(), scalarSrc.address(),
         scalarDst.address(), pressureA_.address(), pressureB_.address());
    vkCmdDispatch(commandBuffer, groups, 1, 1);
    ComputeBarrier(commandBuffer);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, forces_.get());
    push(commandBuffer, velDst.address(), velSrc.address(), scalarSrc.address(),
         scalarDst.address(), pressureA_.address(), pressureB_.address());
    vkCmdDispatch(commandBuffer, groups, 1, 1);
    ComputeBarrier(commandBuffer);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, divergence_.get());
    push(commandBuffer, velSrc.address(), velDst.address(), scalarSrc.address(),
         scalarDst.address(), pressureA_.address(), pressureB_.address());
    vkCmdDispatch(commandBuffer, groups, 1, 1);
    ComputeBarrier(commandBuffer);

    VkDeviceAddress pressureSrc = pressureB_.address();
    VkDeviceAddress pressureDst = pressureA_.address();
    for (std::uint32_t iter = 0; iter < JacobiIterations; ++iter) {
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, jacobi_.get());
      push(commandBuffer, velSrc.address(), velDst.address(), scalarSrc.address(),
           scalarDst.address(), pressureSrc, pressureDst);
      vkCmdDispatch(commandBuffer, groups, 1, 1);
      ComputeBarrier(commandBuffer);
      std::swap(pressureSrc, pressureDst);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, project_.get());
    push(commandBuffer, velSrc.address(), velDst.address(), scalarSrc.address(),
         scalarDst.address(), pressureSrc, pressureDst);
    vkCmdDispatch(commandBuffer, groups, 1, 1);
    ComputeBarrier(commandBuffer);
    velocityFlip_ = !velocityFlip_;

    const gpu::Buffer& velNow = velocityFront();
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, advectScalar_.get());
    push(commandBuffer, velNow.address(), velNow.address(), scalarSrc.address(),
         scalarDst.address(), pressureSrc, pressureDst);
    vkCmdDispatch(commandBuffer, groups, 1, 1);
    ComputeBarrier(commandBuffer);
    scalarFlip_ = !scalarFlip_;
  }
  gpu::GlobalBarrier(
      commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
}

void SmokeSolver::step(std::uint32_t steps) {
  gpu::ImmediateSubmit(*context_,
                       [&](VkCommandBuffer commandBuffer) { record(commandBuffer, steps); });
}

void SmokeSolver::downloadScalar(std::span<glm::vec2> dest) const {
  scalarFront().invalidate();
  const auto src = scalarFront().mapped<const glm::vec2>();
  const std::size_t count = std::min(dest.size(), src.size());
  std::copy_n(src.begin(), count, dest.begin());
}

void SmokeSolver::downloadVelocity(std::span<glm::vec4> dest) const {
  velocityFront().invalidate();
  const auto src = velocityFront().mapped<const glm::vec4>();
  const std::size_t count = std::min(dest.size(), src.size());
  std::copy_n(src.begin(), count, dest.begin());
}

}  // namespace sph::sim
