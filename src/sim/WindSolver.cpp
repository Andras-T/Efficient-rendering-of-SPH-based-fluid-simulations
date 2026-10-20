#include "sim/WindSolver.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace sph::sim {
namespace {

constexpr std::uint32_t WindGroupSize = 64;

struct LbmPop {
  std::array<float, 20> f{};
};

static_assert(sizeof(LbmPop) == 80);

struct WindPush {
  VkDeviceAddress src = 0;
  VkDeviceAddress dst = 0;
  VkDeviceAddress occupancy = 0;
  VkDeviceAddress macro = 0;
  VkDeviceAddress tracers = 0;
  glm::uvec3 grid{1, 1, 1};
  std::uint32_t cellCount = 0;
  float tau = 0.60F;
  float inletUx = 0.08F;
  std::uint32_t tracerCount = 0;
  std::uint32_t seed = 1;
  glm::vec3 origin{0.0F};
  float cellSize = 0.04F;
};

static_assert(sizeof(WindPush) == 88);

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

std::array<float, 19> Equilibrium(float density, glm::vec3 velocity) {
  constexpr std::array Ex{0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0};
  constexpr std::array Ey{0, 0, 0, 1, -1, 0, 0, 1, 1, -1, -1, 0, 0, 0, 0, 1, -1, 1, -1};
  constexpr std::array Ez{0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, 1, -1, -1, 1, 1, -1, -1};
  constexpr std::array W{1.0F / 3.0F,  1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F,
                         1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F,
                         1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F,
                         1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F};
  const float uu = glm::dot(velocity, velocity);
  std::array<float, 19> feq{};
  for (int k = 0; k < 19; ++k) {
    const glm::vec3 e{static_cast<float>(Ex.at(static_cast<std::size_t>(k))),
                      static_cast<float>(Ey.at(static_cast<std::size_t>(k))),
                      static_cast<float>(Ez.at(static_cast<std::size_t>(k)))};
    const float eu = glm::dot(e, velocity);
    feq.at(static_cast<std::size_t>(k)) = W.at(static_cast<std::size_t>(k)) * density *
                                          (1.0F + 3.0F * eu + 4.5F * eu * eu - 1.5F * uu);
  }
  return feq;
}

}  // namespace

WindSolver::WindSolver(const gpu::Context& context)
    : context_(&context), cellCount_(domain_.grid.x * domain_.grid.y * domain_.grid.z) {
  const std::array pushConstants{VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .size = sizeof(WindPush),
  }};
  layout_ = gpu::CreatePipelineLayout(context.device(), pushConstants);
  const VkDevice device = context.device();
  collide_ = MakeCompute(device, layout_.get(), "LbmCollide.comp");
  stream_ = MakeCompute(device, layout_.get(), "LbmStream.comp");
  tracersCompute_ = MakeCompute(device, layout_.get(), "WindTracers.comp");
  current_ = MakeStorage(context, static_cast<VkDeviceSize>(cellCount_) * sizeof(LbmPop));
  scratch_ = MakeStorage(context, static_cast<VkDeviceSize>(cellCount_) * sizeof(LbmPop));
  occupancy_ = MakeStorage(context, static_cast<VkDeviceSize>(cellCount_) * sizeof(std::uint32_t));
  macro_ = MakeStorage(context, static_cast<VkDeviceSize>(cellCount_) * sizeof(glm::vec4));
  tracers_ = MakeStorage(context, static_cast<VkDeviceSize>(TracerCount) * sizeof(Particle));
  const auto mesh = BuildFormulaCarMesh();
  meshCount_ = static_cast<std::uint32_t>(mesh.size());
  mesh_ = MakeStorage(context, mesh.size() * sizeof(MeshVertex));
  std::memcpy(mesh_.mapped<std::uint8_t>().data(), mesh.data(), mesh.size() * sizeof(MeshVertex));
  mesh_.flush();
  initState();
}

void WindSolver::setInletSpeed(float latticeSpeed) {
  inletUx_ = std::clamp(latticeSpeed, 0.02F, 0.14F);
}

void WindSolver::initState() {
  const auto occupancy = BuildWindOccupancy(domain_);
  std::memcpy(occupancy_.mapped<std::uint8_t>().data(), occupancy.data(),
              occupancy.size() * sizeof(std::uint32_t));
  occupancy_.flush();

  const auto feqRest = Equilibrium(1.0F, {});
  const auto feqInlet = Equilibrium(1.0F, {inletUx_, 0.0F, 0.0F});
  auto cells = current_.mapped<LbmPop>();
  auto macros = macro_.mapped<glm::vec4>();
  for (std::uint32_t i = 0; i < cellCount_; ++i) {
    LbmPop pop{};
    const std::uint32_t occ = occupancy[i];
    const auto& feq = occ == static_cast<std::uint32_t>(WindCell::Inlet) ? feqInlet : feqRest;
    for (int k = 0; k < 19; ++k) {
      pop.f.at(static_cast<std::size_t>(k)) = feq.at(static_cast<std::size_t>(k));
    }
    cells[i] = pop;
    const float ux = occ == static_cast<std::uint32_t>(WindCell::Solid) ||
                             occ == static_cast<std::uint32_t>(WindCell::Wall)
                         ? 0.0F
                         : inletUx_ * 0.25F;
    macros[i] = {1.0F, ux, 0.0F, 0.0F};
  }
  current_.flush();
  macro_.flush();
  std::memcpy(scratch_.mapped<std::uint8_t>().data(), current_.mapped<std::uint8_t>().data(),
              current_.size());
  scratch_.flush();

  auto tracers = tracers_.mapped<Particle>();
  const glm::vec3 extent = domain_.cellSize * glm::vec3(domain_.grid);
  for (std::uint32_t i = 0; i < TracerCount; ++i) {
    Particle particle{};
    const std::uint32_t ix = i % 4U;
    const std::uint32_t iy = (i / 4U) % 32U;
    const std::uint32_t iz = i / 128U;
    const float tx = 0.05F + 0.03F * (static_cast<float>(ix) / 3.0F);
    const float ty = 0.14F + 0.52F * (static_cast<float>(iy) / 31.0F);
    const float tz = 0.14F + 0.72F * (static_cast<float>(iz) / 63.0F);
    particle.position = domain_.origin + glm::vec3(tx * extent.x, ty * extent.y, tz * extent.z);
    particle.velocity = {inletUx_, 0.0F, 0.0F};
    particle.alpha = 0.3F + 1.4F * (static_cast<float>(i % 17) / 16.0F);
    tracers[i] = particle;
  }
  tracers_.flush();
  seed_ = 1;
}

void WindSolver::reset() { initState(); }

void WindSolver::push(VkCommandBuffer commandBuffer, bool collide) const {
  const WindPush constants{
      .src = collide ? current_.address() : scratch_.address(),
      .dst = collide ? scratch_.address() : current_.address(),
      .occupancy = occupancy_.address(),
      .macro = macro_.address(),
      .tracers = tracers_.address(),
      .grid = domain_.grid,
      .cellCount = cellCount_,
      .tau = tau_,
      .inletUx = inletUx_,
      .tracerCount = TracerCount,
      .seed = seed_,
      .origin = domain_.origin,
      .cellSize = domain_.cellSize,
  };
  vkCmdPushConstants(commandBuffer, layout_.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(constants), &constants);
}

void WindSolver::record(VkCommandBuffer commandBuffer, std::uint32_t steps) {
  const std::uint32_t cellGroups = (cellCount_ + WindGroupSize - 1) / WindGroupSize;
  const std::uint32_t tracerGroups = (TracerCount + WindGroupSize - 1) / WindGroupSize;
  for (std::uint32_t step = 0; step < steps; ++step) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, collide_.get());
    push(commandBuffer, true);
    vkCmdDispatch(commandBuffer, cellGroups, 1, 1);
    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, stream_.get());
    push(commandBuffer, false);
    vkCmdDispatch(commandBuffer, cellGroups, 1, 1);
    gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
    ++seed_;
  }
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, tracersCompute_.get());
  push(commandBuffer, true);
  vkCmdDispatch(commandBuffer, tracerGroups, 1, 1);
  gpu::GlobalBarrier(
      commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
}

void WindSolver::step(std::uint32_t steps) {
  gpu::ImmediateSubmit(*context_,
                       [&](VkCommandBuffer commandBuffer) { record(commandBuffer, steps); });
}

void WindSolver::downloadMacro(std::span<glm::vec4> dest) const {
  macro_.invalidate();
  const auto src = macro_.mapped<const glm::vec4>();
  const std::size_t count = std::min(dest.size(), src.size());
  std::copy_n(src.begin(), count, dest.begin());
}

void WindSolver::downloadTracers(std::span<Particle> dest) const {
  tracers_.invalidate();
  const auto src = tracers_.mapped<const Particle>();
  const std::size_t count = std::min(dest.size(), src.size());
  std::copy_n(src.begin(), count, dest.begin());
}

}  // namespace sph::sim
