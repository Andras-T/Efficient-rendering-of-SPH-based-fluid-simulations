#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

#include "BufferManager/BufferManager.h"
#include "DescriptorManager/DescriptorManager.h"
#include "DeviceManager/DeviceManager.h"
#include "Utils/Structs/UniformData.h"

class FluidInstance {

private:
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkDescriptorSet> quadDescriptorSets;

  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;

  std::vector<VkBuffer> attributesUniformBuffers;
  std::vector<VkDeviceMemory> attributesUniformBuffersMemory;
  std::vector<void *> attributesUniformBuffersMapped;

  std::vector<VkBuffer> modelUniformBuffers;
  std::vector<VkDeviceMemory> modelUniformBuffersMemory;
  std::vector<void *> modelUniformBuffersMapped;

  glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);

  GLFWwindow *window;
  BufferManager *bufferManager;
  Logger &logger;

public:
  FluidInstance() : logger(Logger::getInstance()) {}

  void InitBuffers(BufferManager &bufferManager, DeviceManager &deviceManager,
                   VkCommandPool &commandPool, glm::vec3 center,
                   GLFWwindow *window);

  void resetInstance(BufferManager &bufferManager, DeviceManager &deviceManager,
                     VkCommandPool &commandPool);

  void InitDescriptorSets(DescriptorManager &descriptorManager,
                          VkDevice &device, BufferManager &bufferManager);

  void Render(const VkCommandBuffer &commandBuffer, VkPipelineLayout &layout,
              uint32_t currentFrame);

  void updateUniformBuffer(uint32_t currentImage, uint32_t lastFrameTime,
                           VkExtent2D *extent2D, UniformData &uniformData,
                           InputState &inputState);

  void cleanUp(VkDevice &device);

  std::vector<VkDescriptorSet> &getDescriptorSets() { return descriptorSets; }

  glm::vec3 getCenter() { return glm::vec3(center.x, center.y, center.z); }

  std::vector<VkDescriptorSet> &getQuadDescriptorSets() {
    return quadDescriptorSets;
  }
};
