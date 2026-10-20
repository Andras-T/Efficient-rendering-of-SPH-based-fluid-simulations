#include "render/Renderer.hpp"

#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "sim/Grid.hpp"
#include "sim/Particle.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#ifndef SPH_HDRI_PATH
#define SPH_HDRI_PATH ""
#endif

namespace sph::render {
namespace {

void SetViewport(VkCommandBuffer commandBuffer, VkExtent2D extent) {
  const VkViewport viewport{
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
      .maxDepth = 1.0F,
  };
  const VkRect2D scissor{.extent = extent};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void BeginColor(VkCommandBuffer commandBuffer, VkImageView view, VkExtent2D extent,
                VkAttachmentLoadOp loadOp, const VkClearValue* clear = nullptr) {
  const VkRenderingAttachmentInfo colorAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = loadOp,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear != nullptr ? *clear : VkClearValue{},
  };
  const VkRect2D area{.extent = extent};
  const VkRenderingInfo rendering{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = area,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
  };
  vkCmdBeginRendering(commandBuffer, &rendering);
  SetViewport(commandBuffer, extent);
}

void BeginColorDepth(VkCommandBuffer commandBuffer, VkImageView color, VkImageView depth,
                     VkExtent2D extent, VkAttachmentLoadOp colorLoad,
                     VkAttachmentLoadOp depthLoad) {
  const VkClearValue colorClear{.color = {.float32 = {0.0F, 0.0F, 0.0F, 0.0F}}};
  const VkClearValue depthClear{.depthStencil = {.depth = 0.0F}};
  const VkRenderingAttachmentInfo colorAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = color,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = colorLoad,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = colorClear,
  };
  const VkRenderingAttachmentInfo depthAttachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = depthLoad,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depthClear,
  };
  const VkRect2D area{.extent = extent};
  const VkRenderingInfo rendering{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = area,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
  };
  vkCmdBeginRendering(commandBuffer, &rendering);
  SetViewport(commandBuffer, extent);
}

}  // namespace

Renderer::Renderer(const gpu::Context& context, VkFormat colorFormat)
    : context_(&context),
      hdri_(context, SPH_HDRI_PATH),
      skyPass_(context, HdrFormat, hdri_),
      particlePass_(context, HdrFormat, DepthFormat),
      fluidPass_(context, HdrFormat, DepthFormat, hdri_),
      foamPass_(context, HdrFormat, DepthFormat),
      tankPass_(context, HdrFormat, DepthFormat, hdri_),
      paddlePass_(context, HdrFormat, DepthFormat),
      tonemapPass_(context, colorFormat),
      hdrSampler_(gpu::CreateSampler(context.device(), VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)),
      uniforms_(context,
                {
                    .size = sizeof(sim::FrameUniforms),
                    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT,
                }) {}

void Renderer::resize(VkExtent2D extent) {
  if (extent.width == 0 || extent.height == 0) {
    return;
  }
  if (hdr_.handle() != VK_NULL_HANDLE && hdr_.extent().width == extent.width &&
      hdr_.extent().height == extent.height) {
    return;
  }
  hdr_ = gpu::Image(*context_,
                    {
                        .extent = extent,
                        .format = HdrFormat,
                        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    });
  depth_ = gpu::Image(*context_, {
                                     .extent = extent,
                                     .format = DepthFormat,
                                     .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                     .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
                                 });
  fluidPass_.resize(extent);
  tonemapPass_.setSource(context_->device(), hdr_.view(), hdrSampler_.get());
}

void Renderer::record(VkCommandBuffer commandBuffer, const RenderTarget& target,
                      const Camera& camera, VkDeviceAddress particles, std::uint32_t particleCount,
                      VkDeviceAddress foam, std::uint32_t foamCapacity,
                      const sim::SimParams& params, const OverlayCallback& overlay) {
  resize(target.extent);

  const sim::Grid grid = sim::MakeGrid(params);
  sim::FrameUniforms uniforms;
  uniforms.view = camera.view();
  uniforms.projection = camera.projection();
  uniforms.inverseView = glm::inverse(uniforms.view);
  uniforms.inverseProjection = glm::inverse(uniforms.projection);
  uniforms.lightDirection = glm::normalize(uniforms.lightDirection);
  uniforms.radius = params.renderRadius();
  uniforms.particles = particles;
  uniforms.count = particleCount;
  uniforms.debugView = static_cast<std::uint32_t>(settings_.view);
  uniforms.absorption = settings_.absorption;
  uniforms.ior = settings_.ior;
  uniforms.domainMin = params.domainMin;
  uniforms.restDensity = params.restDensity;
  uniforms.domainMax = params.domainMax;
  uniforms.invCellSize = 1.0F / grid.cellSize;
  uniforms.gridSize = grid.size;
  uniforms.exposure = settings_.exposure;
  uniforms.wallDisplay = static_cast<std::uint32_t>(settings_.walls);
  uniforms_.mapped<sim::FrameUniforms>().front() = uniforms;
  uniforms_.flush();
  const VkDeviceAddress frameAddress = uniforms_.address();

  gpu::TransitionImage(commandBuffer,
                       {
                           .image = hdr_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                       });
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = depth_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                           .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                           .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
                       });
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = target.image,
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                       });

  BeginColor(commandBuffer, hdr_.view(), target.extent, VK_ATTACHMENT_LOAD_OP_DONT_CARE);
  skyPass_.record(commandBuffer, frameAddress);
  vkCmdEndRendering(commandBuffer);

  const float foamRadius = params.renderRadius() * 0.42F;
  const bool glass = settings_.walls == WallDisplay::Glass;
  const bool wallParticles = settings_.walls == WallDisplay::Particles;
  const bool showPaddle = settings_.paddleRadius > 0.0F;
  const auto recordPaddle = [&] {
    if (showPaddle) {
      paddlePass_.record(commandBuffer, frameAddress, settings_.paddleCenter,
                         settings_.paddleRadius);
    }
  };

  if (settings_.view == DebugView::Foam) {
    BeginColorDepth(commandBuffer, hdr_.view(), depth_.view(), target.extent,
                    VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR);
    if (glass) {
      tankPass_.recordBack(commandBuffer, frameAddress);
    }
    foamPass_.record(commandBuffer, frameAddress, foam, foamCapacity, foamRadius);
    recordPaddle();
    if (glass) {
      tankPass_.recordFront(commandBuffer, frameAddress);
    }
    vkCmdEndRendering(commandBuffer);
  } else if (IsParticleView(settings_.view)) {
    BeginColorDepth(commandBuffer, hdr_.view(), depth_.view(), target.extent,
                    VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR);
    if (glass) {
      tankPass_.recordBack(commandBuffer, frameAddress);
    }
    particlePass_.record(commandBuffer, frameAddress, particleCount);
    recordPaddle();
    if (glass) {
      tankPass_.recordFront(commandBuffer, frameAddress);
    }
    vkCmdEndRendering(commandBuffer);
  } else {
    if (glass) {
      BeginColorDepth(commandBuffer, hdr_.view(), depth_.view(), target.extent,
                      VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR);
      tankPass_.recordBack(commandBuffer, frameAddress);
      vkCmdEndRendering(commandBuffer);
    }
    fluidPass_.prepareTargets(commandBuffer);
    fluidPass_.recordDepth(commandBuffer, frameAddress, particleCount, depth_.view(),
                           target.extent);
    fluidPass_.recordThickness(commandBuffer, frameAddress, particleCount);
    fluidPass_.recordSmooth(commandBuffer);
    if (settings_.view == DebugView::Final && wallParticles) {
      BeginColorDepth(commandBuffer, hdr_.view(), depth_.view(), target.extent,
                      VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
      particlePass_.record(commandBuffer, frameAddress, particleCount);
      vkCmdEndRendering(commandBuffer);
    }
    BeginColor(commandBuffer, hdr_.view(), target.extent, VK_ATTACHMENT_LOAD_OP_LOAD);
    fluidPass_.recordComposite(commandBuffer, frameAddress);
    vkCmdEndRendering(commandBuffer);
    if (settings_.view == DebugView::Final) {
      BeginColorDepth(commandBuffer, hdr_.view(), depth_.view(), target.extent,
                      VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
      foamPass_.record(commandBuffer, frameAddress, foam, foamCapacity, foamRadius);
      vkCmdEndRendering(commandBuffer);
    }
    if (glass || showPaddle) {
      BeginColorDepth(commandBuffer, hdr_.view(), depth_.view(), target.extent,
                      VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
      recordPaddle();
      if (glass) {
        tankPass_.recordFront(commandBuffer, frameAddress);
      }
      vkCmdEndRendering(commandBuffer);
    }
  }

  gpu::TransitionImage(commandBuffer,
                       {
                           .image = hdr_.handle(),
                           .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                           .dstAccess = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                       });

  BeginColor(commandBuffer, target.view, target.extent, VK_ATTACHMENT_LOAD_OP_DONT_CARE);
  tonemapPass_.record(commandBuffer, frameAddress);
  vkCmdEndRendering(commandBuffer);

  if (overlay) {
    BeginColor(commandBuffer, target.view, target.extent, VK_ATTACHMENT_LOAD_OP_LOAD);
    overlay(commandBuffer);
    vkCmdEndRendering(commandBuffer);
  }
}

}  // namespace sph::render
