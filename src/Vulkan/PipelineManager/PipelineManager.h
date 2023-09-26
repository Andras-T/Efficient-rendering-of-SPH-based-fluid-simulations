#ifndef PIPELINEMANAGER_H
#define PIPELINEMANAGER_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <array>
#include <string>
#include <vector>

class PipelineManager {

  VkPipelineLayout simulationPipelineLayout;
  VkPipeline simulationPipeline;

  VkPipelineLayout computePipelineLayout;
  VkPipeline computePipeline;

  VkPipelineLayout quadPipelineLayout;
  VkPipeline quadGraphicsPipeline;

public:
  void init(const char *vertPath, const char *fragPath, const char *compPath,
            VkDevice &device, VkDescriptorSetLayout &descriptorSetLayout,
            VkDescriptorSetLayout &quadDescriptorSetLayout,
            VkRenderPass &renderPass);

  void createSimulationPipeline(const char *vertPath, const char *fragPath,
                              VkDevice &device,
                              VkDescriptorSetLayout &descriptorSetLayout,
                              VkRenderPass &renderPass);

  void createComputePipeline(const char *compPath, VkDevice &device,
                             VkDescriptorSetLayout &descriptorSetLayout,
                             VkRenderPass &renderPass);

  void createQuadGraphicsPipeline(VkDevice &device,
                                  VkDescriptorSetLayout &descriptorSetLayout,
                                  VkRenderPass &renderPass);

  VkShaderModule createShaderModule(const std::vector<char> &code,
                                    VkDevice &device);

  void cleanup(VkDevice &device);

#pragma region getterFunctions

  VkPipelineLayout &getSimulationPipelineLayout() { return simulationPipelineLayout; }

  VkPipelineLayout &getComputePipelineLayout() { return computePipelineLayout; }

  VkPipelineLayout &getQuadPipelineLayout() { return quadPipelineLayout; }

  VkPipeline &getSimulationPipeline() { return simulationPipeline; }

  VkPipeline &getComputePipeline() { return computePipeline; }

  VkPipeline &getQuadGraphicsPipeline() { return quadGraphicsPipeline; }

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

  VkPipelineVertexInputStateCreateInfo getVertexInputStateCreateInfo(
      VkVertexInputBindingDescription &bindingDescription,
      std::array<VkVertexInputAttributeDescription, 2> &attributeDescriptions);

#pragma endregion
};

#endif // PIPELINEMANAGER_H
