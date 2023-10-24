#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "PipelineManager.h"
#include "../Utils/Structs/Particle.h"
#include "../Utils/Structs/Quad.h"
#include "../Utils/Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

void PipelineManager::init(VkDevice &device,
                           VkDescriptorSetLayout &descriptorSetLayout,
                           VkDescriptorSetLayout &quadDescriptorSetLayout,
                           VkDescriptorSetLayout &blurDescriptorSetLayout,
                           VulkanObject &vulkanObject) {
  // Off screen pipeline
  {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(glm::vec4);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    DepthStencilOptions depthStencilOptions(VK_TRUE, VK_TRUE, VK_TRUE,
                                            VK_FALSE);
    simulationPipeline.init(
        &device, descriptorSetLayout, vulkanObject.getSimulationRenderPass(),
        bindingDescription, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        depthStencilOptions);

    logger.LogInfo("Off screen pipeline created");
  }

  // Compute pipeline
  {
    computePipeline.init(&device, descriptorSetLayout,
                         vulkanObject.getSimulationRenderPass());

    logger.LogInfo("Compute pipeline created");
  }

  // Blur pipeline
  {
    DepthStencilOptions depthStencilOptions(VK_FALSE, VK_FALSE, VK_FALSE,
                                            VK_FALSE);

    blurPipeline.init(
        &device, blurDescriptorSetLayout, vulkanObject.getBlurRenderPass(),
        quad::getBindingDescription(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        depthStencilOptions);

    logger.LogInfo("Blur pipeline created");
  }

  // Quad Graphics pipeline
  {
    DepthStencilOptions depthStencilOptions(VK_FALSE, VK_FALSE, VK_FALSE,
                                            VK_FALSE);

    quadPipeline.init(
        &device, quadDescriptorSetLayout, vulkanObject.getQuadRenderPass(),
        quad::getBindingDescription(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        depthStencilOptions);

    logger.LogInfo("Quad pipeline created");
  }
}

void PipelineManager::cleanup(VkDevice &device) {
  simulationPipeline.cleanUp();
  computePipeline.cleanUp();
  quadPipeline.cleanUp();
  blurPipeline.cleanUp();
}
