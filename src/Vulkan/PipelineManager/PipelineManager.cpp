#include "PipelineManager.h"
#include "../Utils/Structs/Particle.h"
#include "../Utils/Structs/Quad.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

void PipelineManager::init(const char *vertPath, const char *fragPath,
                           const char *compPath, VkDevice &device,
                           VkDescriptorSetLayout &descriptorSetLayout,
                           VkDescriptorSetLayout &quadDescriptorSetLayout,
                           VkRenderPass &renderPass) {
  createQuadGraphicsPipeline(device, quadDescriptorSetLayout, renderPass);
  createSimulationPipeline(vertPath, fragPath, device, descriptorSetLayout,
                           renderPass);
  createComputePipeline(compPath, device, descriptorSetLayout, renderPass);
}

void PipelineManager::createSimulationPipeline(
    const char *vertPath, const char *fragPath, VkDevice &device,
    VkDescriptorSetLayout &descriptorSetLayout, VkRenderPass &renderPass) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo =
      getPipelineLayoutCreateInfo(descriptorSetLayout);

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &simulationPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
  std::string currentPath(std::filesystem::current_path().string());
  std::string absVertPath = currentPath + vertPath;
  std::string absFragPath = currentPath + fragPath;

  VkShaderModule vertShaderModule =
      createShaderModule(readFile(absVertPath), device);
  VkShaderModule fragShaderModule =
      createShaderModule(readFile(absFragPath), device);

  VkPipelineShaderStageCreateInfo shaderStages[] = {
      getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                       vertShaderModule, "main"),
      getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                       fragShaderModule, "main")};

  auto bindingDescription = Particle::getBindingDescription();
  auto attributeDescriptions = Particle::getAttributeDescriptions();
  VkPipelineVertexInputStateCreateInfo vertexInputInfo =
      getVertexInputStateCreateInfo(bindingDescription, attributeDescriptions);
  VkPipelineInputAssemblyStateCreateInfo inputAssembly =
      getInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  VkPipelineViewportStateCreateInfo viewportState =
      getViewportStateCreateInfo();
  VkPipelineRasterizationStateCreateInfo rasterizer =
      getRasterizationStateCreateInfo();
  VkPipelineMultisampleStateCreateInfo multisampling =
      getMultisampleStateCreateInfo();
  VkPipelineDepthStencilStateCreateInfo depthStencil =
      getDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_TRUE, VK_TRUE);
  VkPipelineColorBlendAttachmentState colorBlendAttachment =
      getColorBlendAttachmentState(VK_FALSE);
  VkPipelineColorBlendStateCreateInfo colorBlending =
      getColorBlendStateCreateInfo(colorBlendAttachment);

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState =
      getDynamicStateCreateInfo(dynamicStates);

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = simulationPipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &simulationPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void PipelineManager::createComputePipeline(
    const char *compPath, VkDevice &device,
    VkDescriptorSetLayout &descriptorSetLayout, VkRenderPass &renderPass) {
  std::string absCompPath = std::filesystem::current_path().string() + compPath;
  VkShaderModule computeShaderModule =
      createShaderModule(readFile(absCompPath), device);
  VkPipelineShaderStageCreateInfo computeShaderStageInfo =
      getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT,
                                       computeShaderModule, "main");
  VkPushConstantRange pushConstantRange =
      getPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int));
  VkPipelineLayoutCreateInfo pipelineLayoutInfo =
      getPipelineLayoutCreateInfo(descriptorSetLayout, 1, pushConstantRange);

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &computePipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = computePipelineLayout;
  pipelineInfo.stage = computeShaderStageInfo;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                               nullptr, &computePipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }

  vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void PipelineManager::createQuadGraphicsPipeline(
    VkDevice &device, VkDescriptorSetLayout &descriptorSetLayout,
    VkRenderPass &renderPass) {
  std::string currentPath(std::filesystem::current_path().string());
  std::string vertPath = currentPath + "\\shaders\\quadVert.spv";
  std::string fragPath = currentPath + "\\shaders\\quadFrag.spv";

  VkPipelineLayoutCreateInfo pipelineLayoutInfo =
      getPipelineLayoutCreateInfo(descriptorSetLayout);

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &quadPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkShaderModule vertShaderModule =
      createShaderModule(readFile(vertPath), device);
  VkShaderModule fragShaderModule =
      createShaderModule(readFile(fragPath), device);

  VkPipelineShaderStageCreateInfo shaderStages[] = {
      getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                       vertShaderModule, "main"),
      getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                       fragShaderModule, "main")};

  auto bindingDescription = quad::getBindingDescription();
  auto attributeDescriptions = quad::getAttributeDescriptions();
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
  VkPipelineInputAssemblyStateCreateInfo inputAssembly =
      getInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
  VkPipelineViewportStateCreateInfo viewportState =
      getViewportStateCreateInfo();
  VkPipelineRasterizationStateCreateInfo rasterizer =
      getRasterizationStateCreateInfo();
  VkPipelineMultisampleStateCreateInfo multisampling =
      getMultisampleStateCreateInfo();
  VkPipelineDepthStencilStateCreateInfo depthStencil =
      getDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE);
  VkPipelineColorBlendAttachmentState colorBlendAttachment =
      getColorBlendAttachmentState(VK_FALSE);
  VkPipelineColorBlendStateCreateInfo colorBlending =
      getColorBlendStateCreateInfo(colorBlendAttachment);

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState =
      getDynamicStateCreateInfo(dynamicStates);

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = quadPipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &quadGraphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule
PipelineManager::createShaderModule(const std::vector<char> &code,
                                    VkDevice &device) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

VkPipelineLayoutCreateInfo
PipelineManager::getPipelineLayoutCreateInfo(VkDescriptorSetLayout &setLayout) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &setLayout;
  return pipelineLayoutInfo;
}

VkPipelineLayoutCreateInfo PipelineManager::getPipelineLayoutCreateInfo(
    VkDescriptorSetLayout &setLayout, uint32_t pushConstantRangeCount,
    VkPushConstantRange &pushConstantRange) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &setLayout;
  pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  return pipelineLayoutInfo;
}

std::vector<char> PipelineManager::readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  std::cout << "engine: reading file from: " << filename << "\n";
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

VkPushConstantRange
PipelineManager::getPushConstantRange(VkShaderStageFlagBits stageFlagBits,
                                      uint32_t offset, uint32_t size) {
  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = stageFlagBits;
  pushConstantRange.offset = offset;
  pushConstantRange.size = size;
  return pushConstantRange;
}

VkPipelineShaderStageCreateInfo
PipelineManager::getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                                  VkShaderModule module,
                                                  const char *entryPoint) {
  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = stage;
  shaderStageInfo.module = module;
  shaderStageInfo.pName = entryPoint;
  return shaderStageInfo;
}

VkPipelineInputAssemblyStateCreateInfo
PipelineManager::getInputAssemblyCreateInfo(VkPrimitiveTopology topology) {
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  return inputAssembly;
}

VkPipelineViewportStateCreateInfo
PipelineManager::getViewportStateCreateInfo() {
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;
  return viewportState;
}

VkPipelineRasterizationStateCreateInfo
PipelineManager::getRasterizationStateCreateInfo() {
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  return rasterizer;
}

VkPipelineMultisampleStateCreateInfo
PipelineManager::getMultisampleStateCreateInfo() {
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  return multisampling;
}

VkPipelineDepthStencilStateCreateInfo
PipelineManager::getDepthStencilStateCreateInfo(uint32_t enabledepthTest,
                                                uint32_t enabledepthWrite,
                                                uint32_t enabledepthBoundsTest,
                                                uint32_t enableStencilTest) {
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = enabledepthTest;
  depthStencil.depthWriteEnable = enabledepthWrite;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_TRUE;
  depthStencil.minDepthBounds = 0.0f;  // Optional
  depthStencil.maxDepthBounds = 10.0f; // Optional
  return depthStencil;
}

VkPipelineColorBlendAttachmentState
PipelineManager::getColorBlendAttachmentState(uint32_t enableBlend) {
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = enableBlend;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  return colorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo
PipelineManager::getColorBlendStateCreateInfo(
    VkPipelineColorBlendAttachmentState &colorBlendAttachment) {
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;
  return colorBlending;
}

VkPipelineDynamicStateCreateInfo PipelineManager::getDynamicStateCreateInfo(
    std::vector<VkDynamicState> &dynamicStates) {
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();
  return dynamicState;
}

VkPipelineVertexInputStateCreateInfo
PipelineManager::getVertexInputStateCreateInfo(
    VkVertexInputBindingDescription &bindingDescription,
    std::array<VkVertexInputAttributeDescription, 2> &attributeDescriptions) {
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
  return vertexInputInfo;
}

void PipelineManager::cleanup(VkDevice &device) {
  vkDestroyPipeline(device, simulationPipeline, nullptr);
  vkDestroyPipelineLayout(device, simulationPipelineLayout, nullptr);

  vkDestroyPipeline(device, computePipeline, nullptr);
  vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);

  vkDestroyPipeline(device, quadGraphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, quadPipelineLayout, nullptr);
}
