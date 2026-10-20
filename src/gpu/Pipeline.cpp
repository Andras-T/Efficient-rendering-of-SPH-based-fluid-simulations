#include "gpu/Pipeline.hpp"

#include <array>

namespace sph::gpu {

Unique<VkPipelineLayout> CreatePipelineLayout(VkDevice device,
                                              std::span<const VkPushConstantRange> pushConstants,
                                              std::span<const VkDescriptorSetLayout> setLayouts) {
  const VkPipelineLayoutCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<std::uint32_t>(setLayouts.size()),
      .pSetLayouts = setLayouts.data(),
      .pushConstantRangeCount = static_cast<std::uint32_t>(pushConstants.size()),
      .pPushConstantRanges = pushConstants.data(),
  };
  VkPipelineLayout layout = VK_NULL_HANDLE;
  Check(vkCreatePipelineLayout(device, &info, nullptr, &layout), "vkCreatePipelineLayout");
  return {device, layout, vkDestroyPipelineLayout};
}

Unique<VkPipeline> CreateGraphicsPipeline(VkDevice device, const GraphicsPipelineDesc& desc) {
  const auto vertexModule = CreateShaderModule(device, desc.vertexSpirv);
  const auto fragmentModule = CreateShaderModule(device, desc.fragmentSpirv);
  const std::array stages{
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertexModule.get(),
          .pName = "main",
      },
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragmentModule.get(),
          .pName = "main",
      },
  };

  const VkPipelineVertexInputStateCreateInfo vertexInput{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };
  const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = desc.topology,
  };
  const VkPipelineViewportStateCreateInfo viewport{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };
  const VkPipelineRasterizationStateCreateInfo rasterization{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = desc.cullMode,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0F,
  };
  const VkPipelineMultisampleStateCreateInfo multisample{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  const VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
  };

  constexpr VkPipelineColorBlendAttachmentState Opaque{
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  constexpr VkPipelineColorBlendAttachmentState Additive{
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  constexpr VkPipelineColorBlendAttachmentState Alpha{
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  constexpr std::size_t MaxColorAttachments = 8;
  std::array<VkPipelineColorBlendAttachmentState, MaxColorAttachments> attachments{};
  VkPipelineColorBlendAttachmentState blend = Opaque;
  if (desc.additiveBlend) {
    blend = Additive;
  } else if (desc.alphaBlend) {
    blend = Alpha;
  }
  attachments.fill(blend);
  const VkPipelineColorBlendStateCreateInfo colorBlend{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = static_cast<std::uint32_t>(desc.colorFormats.size()),
      .pAttachments = attachments.data(),
  };

  constexpr std::array DynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamic{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<std::uint32_t>(DynamicStates.size()),
      .pDynamicStates = DynamicStates.data(),
  };

  const VkPipelineRenderingCreateInfo rendering{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = static_cast<std::uint32_t>(desc.colorFormats.size()),
      .pColorAttachmentFormats = desc.colorFormats.data(),
      .depthAttachmentFormat = desc.depthFormat,
  };

  const VkGraphicsPipelineCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &rendering,
      .stageCount = static_cast<std::uint32_t>(stages.size()),
      .pStages = stages.data(),
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterization,
      .pMultisampleState = &multisample,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlend,
      .pDynamicState = &dynamic,
      .layout = desc.layout,
  };
  VkPipeline pipeline = VK_NULL_HANDLE;
  Check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline),
        "vkCreateGraphicsPipelines");
  return {device, pipeline, vkDestroyPipeline};
}

Unique<VkPipeline> CreateComputePipeline(VkDevice device, const ComputePipelineDesc& desc) {
  const auto module = CreateShaderModule(device, desc.spirv);
  const VkComputePipelineCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage =
          {
              .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              .stage = VK_SHADER_STAGE_COMPUTE_BIT,
              .module = module.get(),
              .pName = "main",
          },
      .layout = desc.layout,
  };
  VkPipeline pipeline = VK_NULL_HANDLE;
  Check(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline),
        "vkCreateComputePipelines");
  return {device, pipeline, vkDestroyPipeline};
}

}  // namespace sph::gpu
