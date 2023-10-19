#pragma once

#include "glm/gtc/matrix_transform.hpp"

struct UniformBufferObject {
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 proj = glm::mat4(1.0f);
  glm::vec3 cameraPos = glm::vec3(0.0f, -0.002f, -0.00015f);
  float deltaTime = 1.0f;
};
