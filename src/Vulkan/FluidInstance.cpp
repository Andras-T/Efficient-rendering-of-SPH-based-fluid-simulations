#include "FluidInstance.h"

#include <chrono>
#include <iostream>

#include "Utils/Structs/Attributes.h"
#include "Utils/Structs/Model.h"
#include "Utils/Structs/Transformations.h"
#include "Utils/Structs/UniformBufferObject.h"
#include "Utils/Utils.h"

void FluidInstance::InitBuffers(BufferManager &bufferManager,
                                DeviceManager &deviceManager,
                                VkCommandPool &commandPool, glm::vec3 center) {
  bufferManager.createShaderStorageBuffers(deviceManager, commandPool,
                                           shaderStorageBuffers,
                                           shaderStorageBuffersMemory, center);
  bufferManager.createUniformBuffers(
      deviceManager.getDevice(), deviceManager.getPhysicalDevice(),
      uniformBuffers, uniformBuffersMemory, uniformBuffersMapped,
      sizeof(UniformBufferObject));
  bufferManager.createUniformBuffers(
      deviceManager.getDevice(), deviceManager.getPhysicalDevice(),
      attributesUniformBuffers, attributesUniformBuffersMemory,
      attributesUniformBuffersMapped, sizeof(Attributes));
  bufferManager.createUniformBuffers(
      deviceManager.getDevice(), deviceManager.getPhysicalDevice(),
      modelUniformBuffers, modelUniformBuffersMemory, modelUniformBuffersMapped,
      sizeof(Model));
  this->center = glm::vec4(center, 1.0f);
}

void FluidInstance::InitDescriptorSets(DescriptorManager &descriptorManager,
                                       VkDevice &device,
                                       BufferManager &bufferManager) {
  descriptorManager.createDescriptorSets(
      device, descriptorSets, shaderStorageBuffers,
      bufferManager.getSphereBuffer(), uniformBuffers, attributesUniformBuffers,
      modelUniformBuffers);
}

void FluidInstance::Render(const VkCommandBuffer &commandBuffer,
                           VkPipelineLayout &layout, uint32_t currentFrame) {
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1,
                         &shaderStorageBuffers[currentFrame], offsets);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          layout, 0, 1, &descriptorSets[currentFrame], 0,
                          nullptr);

  vkCmdDraw(commandBuffer, 336, Utils::PARTICLE_COUNT, 0, 0);
}

void FluidInstance::updateUniformBuffer(uint32_t currentImage,
                                        uint32_t lastFrameTime,
                                        VkExtent2D *extent2D,
                                        UniformData &uniformData,
                                        InputState &inputState) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   currentTime - startTime)
                   .count();
  startTime = currentTime;

  Transformations transformations = uniformData.transformations;

  glm::mat4 view(1.0f);
  if (inputState.freeCam) {
    view = Utils::updateCamera(time, inputState, uniformData.io);
  } else {
    glm::vec3 cameraPos = inputState.fixedCamPos + glm::vec3(0.0f, -5.0f, 2.0f);
    glm::vec3 cameraTarget = inputState.fixedCamPos;
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);

    view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
  }

  glm::mat4 translateToCenter = glm::translate(
      glm::mat4(1.0f), glm::vec3(-center.x / 2, -center.y / 2, -center.z / 2));

  glm::mat4 scaleMatrix = glm::scale(
      glm::mat4(1.0f), glm::vec3(transformations.s * transformations.scale));

  glm::mat4 rotationMatrix =
      glm::rotate(glm::mat4(1.0f), glm::radians(transformations.rotations.x),
                  glm::vec3(1.0f, 0.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), glm::radians(transformations.rotations.y),
                  glm::vec3(0.0f, 1.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), glm::radians(transformations.rotations.z),
                  glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 translateBack = glm::translate(
      glm::mat4(1.0f), glm::vec3(center.x / 2, center.y / 2, center.z / 2));

  UniformBufferObject ubo{};
  ubo.model = glm::translate(glm::mat4(1.0f), transformations.translate) *
              translateBack * rotationMatrix * scaleMatrix * translateToCenter;
  // ubo.model = translateBack * rotationMatrix * scaleMatrix *
  // translateToCenter * glm::translate(glm::mat4(1.0f),
  // transformations.translate);
  ubo.view = view;
  ubo.proj =
      glm::perspective(glm::radians(45.0f),
                       extent2D->width / (float)extent2D->height, 0.01f, 15.0f);
  ubo.proj[1][1] *= -1;
  ubo.deltaTime = static_cast<float>(lastFrameTime) * 2.0f;

  uniformData.attributes.center = this->center;

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  memcpy(attributesUniformBuffersMapped[currentImage], &uniformData.attributes,
         sizeof(Attributes));
  memcpy(modelUniformBuffersMapped[currentImage], &uniformData.model,
         sizeof(Model));
}

void FluidInstance::cleanUp(VkDevice &device) {
  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkDestroyBuffer(device, attributesUniformBuffers[i], nullptr);
    vkDestroyBuffer(device, modelUniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    vkFreeMemory(device, attributesUniformBuffersMemory[i], nullptr);
    vkFreeMemory(device, modelUniformBuffersMemory[i], nullptr);
  }

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, shaderStorageBuffers[i], nullptr);
    vkFreeMemory(device, shaderStorageBuffersMemory[i], nullptr);
  }
}