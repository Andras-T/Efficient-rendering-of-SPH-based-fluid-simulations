#include "sim/WindSolver.hpp"

#include "ShaderLibrary.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Pipeline.hpp"
#include "sim/Mesh.hpp"

#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <span>
#include <vector>

namespace sph::sim {
namespace {

constexpr std::uint32_t WindGroupSize = 64;
constexpr float AeroSmooth = 0.18F;

constexpr std::array Ex{0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0};
constexpr std::array Ey{0, 0, 0, 1, -1, 0, 0, 1, 1, -1, -1, 0, 0, 0, 0, 1, -1, 1, -1};
constexpr std::array Ez{0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, 1, -1, -1, 1, 1, -1, -1};
constexpr std::array Opp{0, 2, 1, 4, 3, 6, 5, 10, 9, 8, 7, 14, 13, 12, 11, 18, 17, 16, 15};
constexpr std::array W{1.0F / 3.0F,  1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F,
                       1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F,
                       1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F,
                       1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F, 1.0F / 36.0F};

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
  cpuMesh_ = BuildFormulaCarMesh();
  uploadMesh();
  initState();
}

void WindSolver::uploadMesh() {
  if (mesh_.handle() != VK_NULL_HANDLE) {
    context_->waitIdle();
  }
  const std::size_t count = std::max(cpuMesh_.size(), std::size_t{1});
  meshCount_ = static_cast<std::uint32_t>(cpuMesh_.size());
  mesh_ = MakeStorage(*context_, count * sizeof(MeshVertex));
  if (!cpuMesh_.empty()) {
    std::memcpy(mesh_.mapped<std::uint8_t>().data(), cpuMesh_.data(),
                cpuMesh_.size() * sizeof(MeshVertex));
    mesh_.flush();
  }
}

void WindSolver::setObstacleMesh(const std::filesystem::path& path) {
  auto mesh = LoadObstacleMesh(path);
  FitMeshToTunnel(mesh, domain_);
  cpuMesh_ = std::move(mesh);
  importedMesh_ = true;
  uploadMesh();
  initState();
}

void WindSolver::useFormulaCar() {
  cpuMesh_ = BuildFormulaCarMesh();
  importedMesh_ = false;
  uploadMesh();
  initState();
}

void WindSolver::setInletSpeed(float latticeSpeed) {
  inletUx_ = std::clamp(latticeSpeed, 0.02F, 0.14F);
}

void WindSolver::initState() {
  const auto occupancy =
      importedMesh_ ? BuildWindOccupancy(domain_, cpuMesh_) : BuildWindOccupancy(domain_);
  std::memcpy(occupancy_.mapped<std::uint8_t>().data(), occupancy.data(),
              occupancy.size() * sizeof(std::uint32_t));
  occupancy_.flush();
  cacheFrontalArea(occupancy);
  aero_ = {};

  const auto feqRest = Equilibrium(1.0F, {});
  const auto feqInlet = Equilibrium(1.0F, {inletUx_, 0.0F, 0.0F});
  auto cells = current_.mapped<LbmPop>();
  auto macros = macro_.mapped<glm::vec4>();
  for (std::uint32_t i = 0; i < cellCount_; ++i) {
    LbmPop pop{};
    const std::uint32_t occ = occupancy[i];
    const bool blocked = occ == static_cast<std::uint32_t>(WindCell::Solid) ||
                         occ == static_cast<std::uint32_t>(WindCell::Wall);
    const auto& feq = blocked ? feqRest : feqInlet;
    for (int k = 0; k < 19; ++k) {
      pop.f.at(static_cast<std::size_t>(k)) = feq.at(static_cast<std::size_t>(k));
    }
    cells[i] = pop;
    macros[i] = {1.0F, blocked ? 0.0F : inletUx_, 0.0F, 0.0F};
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

void WindSolver::cacheFrontalArea(std::span<const std::uint32_t> occupancy) {
  std::vector<std::uint8_t> seen(
      static_cast<std::size_t>(domain_.grid.y) * static_cast<std::size_t>(domain_.grid.z), 0);
  std::uint32_t cells = 0;
  for (std::uint32_t z = 0; z < domain_.grid.z; ++z) {
    for (std::uint32_t y = 0; y < domain_.grid.y; ++y) {
      for (std::uint32_t x = 0; x < domain_.grid.x; ++x) {
        const std::uint32_t index = x + y * domain_.grid.x + z * domain_.grid.x * domain_.grid.y;
        if (occupancy[index] != static_cast<std::uint32_t>(WindCell::Solid)) {
          continue;
        }
        const std::uint32_t silhouette = y + z * domain_.grid.y;
        if (seen.at(silhouette) != 0) {
          continue;
        }
        seen.at(silhouette) = 1;
        ++cells;
      }
    }
  }
  frontalArea_ = static_cast<float>(cells);
}

void WindSolver::refreshAero() {
  current_.invalidate();
  occupancy_.invalidate();
  const auto cells = current_.mapped<const LbmPop>();
  const auto occupancy = occupancy_.mapped<const std::uint32_t>();
  glm::vec3 force{0.0F};
  const int gx = static_cast<int>(domain_.grid.x);
  const int gy = static_cast<int>(domain_.grid.y);
  const int gz = static_cast<int>(domain_.grid.z);
  for (int z = 0; z < gz; ++z) {
    for (int y = 0; y < gy; ++y) {
      for (int x = 0; x < gx; ++x) {
        const std::uint32_t index = static_cast<std::uint32_t>(x) +
                                    static_cast<std::uint32_t>(y) * domain_.grid.x +
                                    static_cast<std::uint32_t>(z) * domain_.grid.x * domain_.grid.y;
        if (occupancy[index] != static_cast<std::uint32_t>(WindCell::Fluid) &&
            occupancy[index] != static_cast<std::uint32_t>(WindCell::Inlet) &&
            occupancy[index] != static_cast<std::uint32_t>(WindCell::Outlet)) {
          continue;
        }
        for (int k = 1; k < 19; ++k) {
          const int nx = x + Ex.at(static_cast<std::size_t>(k));
          const int ny = y + Ey.at(static_cast<std::size_t>(k));
          const int nz = z + Ez.at(static_cast<std::size_t>(k));
          if (nx < 0 || ny < 0 || nz < 0 || nx >= gx || ny >= gy || nz >= gz) {
            continue;
          }
          const std::uint32_t neighbor =
              static_cast<std::uint32_t>(nx) + static_cast<std::uint32_t>(ny) * domain_.grid.x +
              static_cast<std::uint32_t>(nz) * domain_.grid.x * domain_.grid.y;
          if (occupancy[neighbor] != static_cast<std::uint32_t>(WindCell::Solid)) {
            continue;
          }
          const std::size_t dir = static_cast<std::size_t>(k);
          const int bounced = Opp.at(dir);
          const float fi = cells[index].f.at(static_cast<std::size_t>(bounced));
          // Subtract rest equilibrium so an open underbody does not leak p0 into lift.
          force += 2.0F * (fi - W.at(dir)) *
                   glm::vec3(static_cast<float>(Ex.at(dir)), static_cast<float>(Ey.at(dir)),
                             static_cast<float>(Ez.at(dir)));
        }
      }
    }
  }
  const float dynamicPressure = 0.5F * inletUx_ * inletUx_ * std::max(frontalArea_, 1.0F);
  WindAero raw;
  raw.drag = force.x;
  raw.lift = force.y;
  raw.side = force.z;
  raw.cd = force.x / dynamicPressure;
  raw.cl = force.y / dynamicPressure;
  raw.frontalArea = frontalArea_ * domain_.cellSize * domain_.cellSize;
  if (!std::isfinite(raw.drag) || !std::isfinite(raw.lift) || !std::isfinite(raw.cd) ||
      !std::isfinite(raw.cl)) {
    return;
  }
  aero_.drag += AeroSmooth * (raw.drag - aero_.drag);
  aero_.lift += AeroSmooth * (raw.lift - aero_.lift);
  aero_.side += AeroSmooth * (raw.side - aero_.side);
  aero_.cd += AeroSmooth * (raw.cd - aero_.cd);
  aero_.cl += AeroSmooth * (raw.cl - aero_.cl);
  aero_.frontalArea = raw.frontalArea;
}

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
