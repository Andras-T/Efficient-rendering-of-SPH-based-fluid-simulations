#include "FluidInstance.h"

#include <chrono>
#include <iostream>

#include "Uniform/Structs/Attributes.h"
#include "Uniform/Structs/MVP.h"
#include "Uniform/Structs/Model.h"
#include "Uniform/Structs/Transformations.h"
#include "Uniform/Structs/UniformData.h"
#include "Utils/Utils.h"

float gaussian(float x, float sigma) {
  return (1.0 / (sqrt(2.0 * glm::pi<float>()) * sigma)) * exp(-(x * x) / (2.0 * sigma * sigma));
}

void FluidInstance::InitBuffers(BufferManager &bufferManager,
                                DeviceManager &deviceManager,
                                VkCommandPool &commandPool, glm::vec3 center,
                                GLFWwindow *window) {
  auto device = deviceManager.getDevice();
  auto physicalDevice = deviceManager.getPhysicalDevice();

  uniforms.try_emplace("Attributes",
                       Uniform(device, physicalDevice, sizeof(Attributes)));
  uniforms.try_emplace("BlurSettings",
                       Uniform(device, physicalDevice, sizeof(BlurSettings)));
  uniforms.try_emplace("Model", Uniform(device, physicalDevice, sizeof(Model)));
  uniforms.try_emplace("MVP", Uniform(device, physicalDevice, sizeof(MVP)));
  uniforms.try_emplace("ViewMode",
                       Uniform(device, physicalDevice, sizeof(ViewMode)));

  bufferManager.createShaderStorageBuffers(deviceManager, commandPool,
                                           shaderStorageBuffers,
                                           shaderStorageBuffersMemory, center);

  logger.LogInfo("VkBuffers created for uniforms and SSB");
  this->center = center;
  this->window = window;
}

void FluidInstance::resetInstance(BufferManager &bufferManager,
                                  DeviceManager &deviceManager,
                                  VkCommandPool &commandPool) {
  bufferManager.resetParticles(deviceManager, commandPool, shaderStorageBuffers,
                               shaderStorageBuffersMemory);
}

void FluidInstance::InitDescriptorSets(DescriptorManager &descriptorManager,
                                       VkDevice &device,
                                       BufferManager &bufferManager) {
  descriptorManager.createDescriptorSets(device, descriptorSets,
                                         quadDescriptorSets, blurDescriptorSets,
                                         shaderStorageBuffers, uniforms);
  this->bufferManager = &bufferManager;
  logger.LogInfo("Descriptor sets created");
}

void FluidInstance::Render(const VkCommandBuffer &commandBuffer,
                           VkPipelineLayout &layout, uint32_t currentFrame) {
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &bufferManager->getSphereBuffer(),
                         offsets);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          layout, 0, 1, &descriptorSets[currentFrame], 0,
                          nullptr);

  vkCmdDraw(commandBuffer, Utils::getVertexCount(), Utils::PARTICLE_COUNT, 0,
            0);
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
    glm::vec3 cameraPos = inputState.fixedCamPos;
    glm::vec3 cameraTarget = inputState.fixedCamTarget;
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
    inputState.cameraFront = cameraTarget - cameraPos;
    inputState.cameraRight = glm::cross(inputState.cameraFront, inputState.cameraUp);

    view = glm::lookAt(inputState.fixedCamPos, inputState.fixedCamTarget, cameraUp);
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
  glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), center / 2.0f);

  MVP mvp{};
  mvp.model = glm::translate(glm::mat4(1.0f), transformations.translate) *
              translateBack * rotationMatrix * scaleMatrix * translateToCenter;
  mvp.view = view;
  float far = inputState.far;
  float near = inputState.near;
  mvp.proj =
      glm::perspective(glm::radians(45.0f),
                       extent2D->width / (float)extent2D->height, near, far);
  mvp.proj[1][1] *= -1;
  mvp.cameraPos = inputState.cameraPos;
  mvp.deltaTime = static_cast<float>(lastFrameTime) * 2.0f;
  mvp.viewModel = mvp.view * mvp.model;
  mvp.inverseModel = glm::inverse(mvp.model);
  mvp.inverseProj = glm::inverse(mvp.proj);
  mvp.lightPosition = transformations.lightPosition;

  uniformData.attributes.center = this->center;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  uniformData.model.windowSize = glm::vec2(float(width), float(height));
  uniformData.model.farPlaneDistance = far - near;
  
  static float sigma = uniformData.blurSettings.sigma;
  static float kernelSize = uniformData.blurSettings.kernelSize;
  static bool firstTime = true;

  if (sigma != uniformData.blurSettings.sigma || kernelSize != uniformData.blurSettings.kernelSize || firstTime) {
    uniformData.blurSettings.w_sum = 0.0f;
    for (float i = -uniformData.blurSettings.kernelSize; i <= uniformData.blurSettings.kernelSize; i += 1.0) {
      uniformData.blurSettings.w_sum += gaussian(i, uniformData.blurSettings.sigma);
    }
    firstTime = false;
  }
  memcpy(uniforms["MVP"].getBufferMapped(currentImage), &mvp, sizeof(mvp));
  memcpy(uniforms["Attributes"].getBufferMapped(currentImage),
         &uniformData.attributes, sizeof(Attributes));
  memcpy(uniforms["Model"].getBufferMapped(currentImage), &uniformData.model,
         sizeof(Model));
  memcpy(uniforms["BlurSettings"].getBufferMapped(currentImage),
         &uniformData.blurSettings, sizeof(BlurSettings));
  memcpy(uniforms["ViewMode"].getBufferMapped(currentImage),
         &uniformData.viewMode, sizeof(ViewMode));
}

void FluidInstance::cleanUp(VkDevice &device) {
  for (auto &[str, uniform] : uniforms) {
    uniform.cleanUp(device);
  }

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, shaderStorageBuffers[i], nullptr);
    vkFreeMemory(device, shaderStorageBuffersMemory[i], nullptr);
  }
}