#pragma once

#include "glm/vec3.hpp"

struct Transformations {
  glm::vec3 scale = glm::vec3(0.065f);
  glm::vec3 rotations = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 translate = glm::vec3(0.0f);
  glm::vec3 cameraRotations = glm::vec3(0.0f);
  glm::vec4 lightPosition = glm::vec4(1.0, 1.0, 0.0, 1.0);
  float s = 1.0f;
};
