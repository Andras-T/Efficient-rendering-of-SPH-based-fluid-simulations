#pragma once

#include "glm/gtc/matrix_transform.hpp"

struct MVP {
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 proj = glm::mat4(1.0f);
  glm::mat4 viewModel = glm::mat4(1.0f);
  glm::mat4 inverseModel = glm::mat4(1.0f);
  glm::mat4 inverseProj = glm::mat4(1.0f);
  glm::vec3 cameraPos = glm::vec3(0.0f, -0.002f, -0.00015f);
  float deltaTime = 1.0f;
  glm::vec4 lightPosition = glm::vec4(1.0, 1.0, 0.0, 1.0);
};
