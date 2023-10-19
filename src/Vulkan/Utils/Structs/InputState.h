#pragma once

#include "glm/vec3.hpp"

struct InputState {
  glm::vec3 cameraPos = glm::vec3(0.0f, -0.0025f, -0.00015f);
  glm::vec3 cameraFront = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 cameraRotations = glm::vec3(0.0f, 0.0f, 0.0f);
  float cameraSpeed = 0.002f;
  bool freeCam = true;
  glm::vec3 fixedCamTarget = glm::vec3(0.0f, 0.0f, -0.00015f);
};
