#ifndef QUAD_H
#define QUAD_H

#include <array>

struct quad {

  static constexpr std::array<float, 16> quadVertices = {
      -1.0f, -1.0f, 0.0f, 1.0f, // Vertex 1
      1.0f,  -1.0f, 0.0f, 1.0f, // Vertex 2
      -1.0f, 1.0f,  0.0f, 1.0f, // Vertex 3
      1.0f,  1.0f,  0.0f, 1.0f  // Vertex 4
  };

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float) * 4;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 1>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    return attributeDescriptions;
  }
};

#endif // !QUAD_H