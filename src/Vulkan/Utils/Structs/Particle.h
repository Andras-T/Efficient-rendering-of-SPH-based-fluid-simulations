#ifndef PARTICLE_H
#define PARTICLE_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <array>

struct Particle {
  glm::vec4 position;
  glm::vec4 color;
  glm::vec3 velocity;
  float movable;
  glm::vec3 acc;
  float density;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Particle, color);

    return attributeDescriptions;
  }
};

#endif // PARTICLE_H
