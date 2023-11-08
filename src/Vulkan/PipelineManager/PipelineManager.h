#pragma once

#define GLFW_INCLUDE_VULKAN
#include "Logger/Logger.h"
#include "Vulkan/VulkanObject/VulkanObject.h"
#include "GLFW/glfw3.h"
#include "Pipeline.h"
#include <array>
#include <string>
#include <vector>

class PipelineManager {

  std::string vertPath = "\\shaders\\vert.spv";
  std::string fragPath = "\\shaders\\frag.spv";
  std::string compPath = "\\shaders\\comp.spv";
  std::string quadVertPath = "\\shaders\\quadVert.spv";
  std::string quadFragPath = "\\shaders\\quadFrag.spv";
  std::string blurVertPath = "\\shaders\\blurVert.spv";
  std::string blurFragPath = "\\shaders\\blurFrag.spv";

  Pipeline simulationPipeline;
  Pipeline computePipeline;
  Pipeline blurPipeline;
  Pipeline quadPipeline;

  Logger &logger;

public:
  PipelineManager()
      : simulationPipeline(std::string("Simulation"),
                           optional<std::string>(vertPath),
                           optional<std::string>(fragPath),
                           optional<std::string>(std::nullopt)),
        computePipeline(std::string("Compute"),
                        optional<std::string>(std::nullopt),
                        optional<std::string>(std::nullopt),
                        optional<std::string>(compPath)),
        blurPipeline(std::string("Blur"), optional<std::string>(blurVertPath),
                     optional<std::string>(blurFragPath),
                     optional<std::string>(std::nullopt)),
        quadPipeline(std::string("Quad"), optional<std::string>(quadVertPath),
                     optional<std::string>(quadFragPath),
                     optional<std::string>(std::nullopt)),
        logger(Logger::getInstance()) {}

  void init(VkDevice &device, VkDescriptorSetLayout &descriptorSetLayout,
            VkDescriptorSetLayout &quadDescriptorSetLayout,
            VkDescriptorSetLayout &blurDescriptorSetLayout,
            VulkanObject &vulkanObject);

  void cleanup(VkDevice &device);

#pragma region getterFunctions

  VkPipeline &getSimulationPipeline() {
    return simulationPipeline.getPipeline();
  }

  VkPipelineLayout &getSimulationPipelineLayout() {
    return simulationPipeline.getPipelineLayout();
  }

  VkPipeline &getComputePipeline() { return computePipeline.getPipeline(); }

  VkPipelineLayout &getComputePipelineLayout() {
    return computePipeline.getPipelineLayout();
  }

  VkPipeline &getQuadPipeline() { return quadPipeline.getPipeline(); }

  VkPipelineLayout &getQuadPipelineLayout() {
    return quadPipeline.getPipelineLayout();
  }

  VkPipeline &getBlurPipeline() { return blurPipeline.getPipeline(); }

  VkPipelineLayout &getBlurPipelineLayout() {
    return blurPipeline.getPipelineLayout();
  }

#pragma endregion

#pragma region utilFunctions

private:
  VkPipelineLayoutCreateInfo
  getPipelineLayoutCreateInfo(VkDescriptorSetLayout &setLayout);

  VkPipelineLayoutCreateInfo
  getPipelineLayoutCreateInfo(VkDescriptorSetLayout &setLayout,
                              uint32_t pushConstantRangeCount,
                              VkPushConstantRange &pushConstantRange);

  std::vector<char> readFile(const std::string &filename);

  VkPushConstantRange getPushConstantRange(VkShaderStageFlagBits stageFlagBits,
                                           uint32_t offset, uint32_t size);

  VkPipelineShaderStageCreateInfo
  getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                   VkShaderModule module,
                                   const char *entryPoint);

  VkPipelineInputAssemblyStateCreateInfo
  getInputAssemblyCreateInfo(VkPrimitiveTopology topology);

  VkPipelineViewportStateCreateInfo getViewportStateCreateInfo();

  VkPipelineRasterizationStateCreateInfo getRasterizationStateCreateInfo();

  VkPipelineMultisampleStateCreateInfo getMultisampleStateCreateInfo();

  VkPipelineDepthStencilStateCreateInfo getDepthStencilStateCreateInfo(
      uint32_t enabledepthTest, uint32_t enabledepthWrite,
      uint32_t enabledepthBoundsTest, uint32_t enableStencilTest);

  VkPipelineColorBlendAttachmentState
  getColorBlendAttachmentState(uint32_t enableBlend);

  VkPipelineColorBlendStateCreateInfo getColorBlendStateCreateInfo(
      VkPipelineColorBlendAttachmentState &colorBlendAttachment);

  VkPipelineDynamicStateCreateInfo
  getDynamicStateCreateInfo(std::vector<VkDynamicState> &dynamicStates);

#pragma endregion
};
