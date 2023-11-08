#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm/vec3.hpp"

#include "Logger/Logger.h"

#include "Vulkan/Utils/Structs/DepthStencilOptions.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using std::optional;
using std::string;
using std::string_view;

class Pipeline {

private:
  const std::string name;

  VkPipelineLayout layout;
  VkPipeline pipeline;

  const optional<string> vertPath;
  const optional<string> fragPath;
  const optional<string> compPath;

  VkDevice *device;
  Logger &logger;

public:
  Pipeline(std::string &name, optional<string> vertPath,
           optional<string> fragPath, optional<string> compPath)
      : name(name), logger(Logger::getInstance()), device(device),
        vertPath(vertPath), fragPath(fragPath), compPath(compPath) {}

  void init(VkDevice *device, VkDescriptorSetLayout &descriptorSetLayout,
            VkRenderPass &renderPass,
            VkVertexInputBindingDescription bindingDescription = {},
            VkPrimitiveTopology topology = {},
            DepthStencilOptions depthStencilOptions = {},
            VkBool32 alphaBlendingEnable = VK_FALSE);

  VkPipeline &getPipeline() { return pipeline; }

  VkPipelineLayout &getPipelineLayout() { return layout; }

  void cleanUp() {
    vkDestroyPipeline(*device, pipeline, nullptr);
    vkDestroyPipelineLayout(*device, layout, nullptr);
  }

private:
  std::vector<char> readFile(const std::string &filename);

  VkShaderModule createShaderModule(const std::vector<char> &code);

  VkPipelineShaderStageCreateInfo
  getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                   VkShaderModule module,
                                   const char *entryPoint);
};