#include "Render.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include <iostream>

void Render::init(DeviceManager &deviceManager,
                  SwapchainManager &swapChainManager,
                  CommandPoolManager &commandPoolManager,
                  PipelineManager &pipelineManager, VulkanObject &vulkanObject,
                  Window &window, BufferManager &bufferManager,
                  FluidInstance &instance) {
  this->deviceManager = &deviceManager;
  this->swapChainManager = &swapChainManager;
  this->commandPoolManager = &commandPoolManager;
  this->pipelineManager = &pipelineManager;
  this->vulkanObject = &vulkanObject;
  this->window = &window;
  this->bufferManager = &bufferManager;
  this->instance = &instance;
  createSyncObjects();
  imGuiRender.init(window, &instance);
}

void Render::createSyncObjects() {
  VkDevice &device = deviceManager->getDevice();
  imageAvailableSemaphores.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  simulationFinishedSemaphores.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  computeFinishedSemaphores.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  simulationInFlightFences.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  displayInFlightFences.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  computeInFlightFences.resize(Utils::MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &simulationFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr,
                      &simulationInFlightFences[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &displayInFlightFences[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create graphics synchronization objects for a frame!");
    }
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &computeFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &computeInFlightFences[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create compute synchronization objects for a frame!");
    }
  }
}

void Render::drawFrame(uint32_t lastFrameTime) {
  VkDevice &device = deviceManager->getDevice();

  // Compute submission
  {
    vkWaitForFences(device, 1, &computeInFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    instance->updateUniformBuffer(
        currentFrame, lastFrameTime, &swapChainManager->getSwapChainExtent(),
        imGuiRender.getUniformData(), imGuiRender.getInputState());

    vkResetFences(device, 1, &computeInFlightFences[currentFrame]);

    vkResetCommandBuffer(
        commandPoolManager->getComputeCommandBuffers()[currentFrame], 0);
    recordComputeCommandBuffer(
        commandPoolManager->getComputeCommandBuffers()[currentFrame]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers =
        &commandPoolManager->getComputeCommandBuffers()[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

    if (vkQueueSubmit(deviceManager->getComputeQueue(), 1, &submitInfo,
                      computeInFlightFences[currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit compute command buffer!");
    };
  }

  // Simulation submission
  {
    vkWaitForFences(device, 1, &simulationInFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    vkResetFences(device, 1, &simulationInFlightFences[currentFrame]);

    recordCommandBuffer(commandPoolManager->getCommandBuffers()[currentFrame],
                        currentFrame);

    VkSemaphore waitSemaphores[] = {computeFinishedSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers =
        &commandPoolManager->getCommandBuffers()[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &simulationFinishedSemaphores[currentFrame];

    if (vkQueueSubmit(deviceManager->getGraphicsQueue(), 1, &submitInfo,
                      simulationInFlightFences[currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }
  }

  // Display submission
  {
    vkWaitForFences(device, 1, &displayInFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device, swapChainManager->getSwapChain(), UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      swapChainManager->recreateSwapChain(window->get_GLFW_Window(),
                                          deviceManager->getDevice());
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(device, 1, &displayInFlightFences[currentFrame]);

    recordQuad(commandPoolManager->getQuadCommandBuffers()[currentFrame],
               imageIndex);

    VkSemaphore waitSemaphores[] = {simulationFinishedSemaphores[currentFrame],
                                    imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 2;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers =
        &commandPoolManager->getQuadCommandBuffers()[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = {&renderFinishedSemaphores[currentFrame]};

    if (vkQueueSubmit(deviceManager->getGraphicsQueue(), 1, &submitInfo,
                      displayInFlightFences[currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {swapChainManager->getSwapChain()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(deviceManager->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        Window::framebufferResized) {
      Window::framebufferResized = false;
      swapChainManager->recreateSwapChain(window->get_GLFW_Window(),
                                          deviceManager->getDevice());
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }
  }

  currentFrame = (currentFrame + 1) % Utils::MAX_FRAMES_IN_FLIGHT;
}

void Render::recordComputeCommandBuffer(VkCommandBuffer &commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelineManager->getComputePipeline());

  VkMemoryBarrier2 barrier2 = {};
  barrier2.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
  barrier2.pNext = nullptr;
  barrier2.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
  barrier2.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
  barrier2.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
  barrier2.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR;

  VkDependencyInfo dependencyInfo = {};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.dependencyFlags = 0;
  dependencyInfo.memoryBarrierCount = 1;
  dependencyInfo.pMemoryBarriers = &barrier2;
  dependencyInfo.bufferMemoryBarrierCount = 0;
  dependencyInfo.pBufferMemoryBarriers = nullptr;
  dependencyInfo.imageMemoryBarrierCount = 0;
  dependencyInfo.pImageMemoryBarriers = nullptr;

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineManager->getComputePipelineLayout(), 0, 1,
                          &instance->getDescriptorSets()[currentFrame], 0,
                          nullptr);

  int stageIndex = 1;
  vkCmdPushConstants(commandBuffer, pipelineManager->getComputePipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &stageIndex);
  vkCmdDispatch(commandBuffer, Utils::PARTICLE_COUNT / 256, 1, 1);

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  stageIndex++; // 2
  vkCmdPushConstants(commandBuffer, pipelineManager->getComputePipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &stageIndex);
  vkCmdDispatch(commandBuffer, Utils::PARTICLE_COUNT / 256, 1, 1);

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  stageIndex++; // 3
  vkCmdPushConstants(commandBuffer, pipelineManager->getComputePipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &stageIndex);
  vkCmdDispatch(commandBuffer, Utils::PARTICLE_COUNT / 256, 1, 1);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record compute command buffer!");
  }
}

void Render::recordCommandBuffer(VkCommandBuffer &commandBuffer,
                                 uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vulkanObject->getRenderPass();
  renderPassInfo.framebuffer =
      (swapChainManager->getSwapChainFramebuffers())[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainManager->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineManager->getSimulationPipeline());

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainManager->getSwapChainExtent().width;
  viewport.height = (float)swapChainManager->getSwapChainExtent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainManager->getSwapChainExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  instance->Render(commandBuffer,
                   pipelineManager->getSimulationPipelineLayout(),
                   currentFrame);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void Render::recordQuad(VkCommandBuffer &commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vulkanObject->getRenderPass();
  renderPassInfo.framebuffer =
      (swapChainManager->getQuadSwapChainFramebuffers())[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainManager->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = imGuiRender.backgroundColor;
  ;
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineManager->getQuadGraphicsPipeline());

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainManager->getSwapChainExtent().width;
  viewport.height = (float)swapChainManager->getSwapChainExtent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainManager->getSwapChainExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(bufferManager->getQuadBuffer()),
                         offsets);

  auto descriptorSet = &instance->getQuadDescriptorSets()[currentFrame];
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineManager->getQuadPipelineLayout(), 0, 1,
                          descriptorSet, 0, nullptr);

  vkCmdDraw(commandBuffer, 4, 1, 0, 0);

  imGuiRender.draw(commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void Render::cleanUp() {
  VkDevice &device = deviceManager->getDevice();
  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, simulationFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device, computeFinishedSemaphores[i], nullptr);
    vkDestroyFence(device, simulationInFlightFences[i], nullptr);
    vkDestroyFence(device, displayInFlightFences[i], nullptr);
    vkDestroyFence(device, computeInFlightFences[i], nullptr);
  }
}